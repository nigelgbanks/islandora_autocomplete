<?php

/**
 * @file
 *
 * Functions for administering Autocomplete url's that allow the users to
 * create, update, read, delete Autocomplete URLs.
 */
module_load_include('inc', 'islandora_autocomplete', 'includes/islandora_autocomplete.db');

/**
 * Menu callback; admin menu for autocomplete url's.
 * 
 * @return string
 *   The admin page's HTML.
 */
function islandora_autocomplete_admin() {
  $urls = islandora_autocomplete_db_get_urls();
  $header = array(t('Name'), t('URL'), t('Description'), t('Source'), array('data' => t('Operations'), 'colspan' => '3'));
  $rows = array();
  while ($url = db_fetch_object($urls)) {
    $path = str_replace('%', $url->name, MENU_ISLANDORA_AUTOCOMPLETE_URL);
    $view_path = str_replace('%', $url->name, MENU_ISLANDORA_AUTOCOMPLETE_VIEW_URL);
    $edit_path = str_replace('%', $url->name, MENU_ISLANDORA_AUTOCOMPLETE_EDIT_URL);
    $delete_path = str_replace('%', $url->name, MENU_ISLANDORA_AUTOCOMPLETE_DELETE_URL);
    $row = array(
      l($url->name, $path),
      MENU_ISLANDORA_AUTOCOMPLETE . '/' . $url->name,
      filter_xss_admin($url->description),
      check_plain($url->source),
    );
    $row[] = array('data' => l(t('view'), $view_path));
    $row[] = array('data' => l(t('edit'), $edit_path));
    $row[] = array('data' => l(t('delete'), $delete_path));
    $rows[] = $row;
  }
  if (empty($rows)) {
    $rows[] = array(array('data' => t('No autocomplete url\'s available.'), 'colspan' => '5', 'class' => 'message'));
  }
  return theme('table', $header, $rows);
}

/**
 * Displays the content of the view url.
 * 
 * @param string $name 
 *   The name of Autocomplete URL to display.
 */
function islandora_autocomplete_url_view_form(array &$form_state, $name) {
  module_load_include('inc', 'islandora_autocomplete', 'includes/autocomplete');
  $url = islandora_autocomplete_db_get_url_by_name($name);
  $query = isset($form_state['values']['query']) ? $form_state['values']['query'] : '';
  $header = array(t('Value'), t('Label'));
  $results = islandora_autocomplete_query_source($url->id, $url->source, $query);
  $rows = array();
  foreach ($results as $key => $value) {
    $rows[] = array($key, $value);
  }
  if (empty($rows)) {
    $rows[] = array(array('data' => t('No results.'), 'colspan' => '2', 'class' => 'message'));
  }
  return array(
    'query' => array(
      '#title' => t('Query'),
      '#type' => 'textfield',
      '#default_value' => $query,
    ),
    'submit' => array(
      '#type' => 'submit',
      '#value' => 'Test'
    ),
    'cancel' => array(
      '#value' => l(t('Cancel'), MENU_ISLANDORA_AUTOCOMPLETE_ADMIN)
    ),
    'results' => array(
      '#title' => t('Query Results'),
      '#type' => 'fieldset',
      'table' => array(
        '#type' => 'markup',
        '#value' => theme('table', $header, $rows)
      )
    )
  );
}

/**
 * Rebuilds the View form.
 *
 * @param array $form
 *   The drupal form.
 * @param array $form_state 
 *   The drupal form state. 
 */
function islandora_autocomplete_url_view_form_submit(array $form, array &$form_state) {
  $form_state['rebuild'] = TRUE;
}

/**
 * Form definition for adding/updating and autocomplete url.
 * 
 * @param array $form_state
 *   The drupal form state.
 * @param string $name 
 *   The name of Autocomplete URL to update. If not preset this form will be used to create a new url.
 */
function islandora_autocomplete_url_form(array &$form_state, $name = '') {
  $path = drupal_get_path('module', 'islandora_autocomplete');
  drupal_add_js("$path/js/fix-jquery-update.js", 'theme'); // Hacktacular, fixes issues between jquery update file uploads and AHAH.
  $sources = islandora_autocomplete_get_sources();
  $sources = array_combine($sources, $sources);
  $default_source = array_keys($sources);
  $default_source = $default_source[0];
  if (empty($sources)) {
    drupal_set_message(t('No sources are defined, please install a module like "islandora autocomplete csv" so that a source can be defined.'), 'error');
    return array();
  }
  $url = !empty($name) ? object_as_array(islandora_autocomplete_db_get_url_by_name($name)) : FALSE;
  $values = isset($form_state['values']) ? $form_state['values'] : $url;
  $values['source'] = isset($values['source']) ? $values['source'] : $default_source;
  $hook = "islandora_autocomplete_url_{$values['source']}_form";
  $source_form = function_exists($hook) ? call_user_func($hook, $form_state, $url ? $url['id'] : NULL) : array();
  $source_form = array_merge(array(
    '#title' => t('Source Settings'),
    '#type' => 'item',
    '#prefix' => '<div id="source-form-wrapper">',
    '#suffix' => '</div>'), $source_form);
  $id = array(
    '#type' => 'hidden',
    '#value' => isset($values['id']) ? $values['id'] : NULL,
  );
  return array(
    '#action' => empty($name) ? MENU_ISLANDORA_AUTOCOMPLETE_ADD_URL : str_replace('%', $name, MENU_ISLANDORA_AUTOCOMPLETE_URL),
    '#attributes' => array(
      'enctype' => "multipart/form-data"
    ),
    'id' => $id,
    'url_id' => $id, // Copy of id, to simplify the source forms handling of this value.
    'name' => array(
      '#title' => t('Name'),
      '#description' => t('The machine-readable name of this autocomplete url. This text will be used for constructing the URL of #autocomplete_path. This name must contain only lowercase letters, numbers, and underscores. This name must be unique.'),
      '#type' => 'textfield',
      '#required' => TRUE,
      '#default_value' => isset($values['name']) ? $values['name'] : '',
      '#element_validate' => array('islandora_autocomplete_url_validate_name'),
    ),
    'description' => array(
      '#title' => t('Description'),
      '#description' => t('A brief description of this content type.'),
      '#required' => TRUE,
      '#default_value' => isset($values['description']) ? $values['description'] : '',
      '#type' => 'textarea',
    ),
    'source' => array(
      '#title' => t('Source'),
      '#name' => 'source',
      '#description' => t('This is the source from which the autocomplete url will be populated.'),
      '#type' => 'select',
      '#default_value' => isset($values['source']) ? $values['source'] : '',
      '#options' => $sources,
      '#ahah' => array(
        'path' => MENU_ISLANDORA_AUTOCOMPLETE_AHAH_SOURCE,
        'wrapper' => 'source-form-wrapper',
        'method' => 'replace',
        'effect' => 'fade',
      ),
    ),
    'source_form' => $source_form,
    'submit' => array(
      '#type' => 'submit',
      '#value' => t('Submit')
    ),
    'cancel' => array(
      '#value' => l(t('Cancel'), MENU_ISLANDORA_AUTOCOMPLETE_ADMIN)
    ),
  );
}

/**
 * Validates the 'name' form element, ensures the proper format and that the given name is unique.
 * 
 * @param array $element
 *   The form element to validate.
 * @param array $form_state
 *   The drupal form state.
 * @param array $complete_form 
 *   The drupal form.
 */
function islandora_autocomplete_url_validate_name($element, array &$form_state) {
  $values = $form_state['values'];
  $name = $values['name'];
  $original_url = isset($values['id']) ? islandora_autocomplete_db_get_url($values['id']) : FALSE;
  $check_for_uniqueness = $original_url ? $original_url->name != $name : TRUE;
  if (preg_match('/^[a-z0-9_]*$/', $name) != 1) {
    form_error($element, t('The name must contain only lowercase letters, numbers, and underscores.'));
  }
  else if ($check_for_uniqueness && islandora_autocomplete_db_get_url_by_name($name) !== FALSE) {
    form_error($element, t('The name must be unique.'));
  }
}

/**
 * Validate the submitted form.
 *
 * @param array $form
 *   The drupal form.
 * @param array $form_state 
 *   The drupal form state.
 */
function islandora_autocomplete_url_form_validate(array $form, array &$form_state) {
  $hook = "islandora_autocomplete_url_{$form_state['values']['source']}_form_validate";
  if (function_exists($hook)) {
    call_user_func_array($hook, array($form, &$form_state));
  }
}

/**
 * Either adds a autocomplete url or updates and existing one.
 * 
 * @param array $form
 *   The drupal form.
 * @param array $form_state 
 *   The drupal form state.
 */
function islandora_autocomplete_url_form_submit(array $form, array &$form_state) {
  extract($form_state['values'], EXTR_SKIP);
  if (isset($id)) {
    islandora_autocomplete_url_form_submit_update_source($id, $source);
    islandora_autocomplete_db_update_url($id, $name, $description, $source);
  }
  else {
    if (islandora_autocomplete_db_add_url($name, $description, $source) !== FALSE) {
      $url = islandora_autocomplete_db_get_url_by_name($name);
      $values = &$form_state['values'];
      $values['id'] = $values['url_id'] = $url->id; // For use in the source submit handler.
    }
  }
  islandora_autocomplete_url_source_form_submit($source, $form, $form_state);
  $form_state['redirect'] = MENU_ISLANDORA_AUTOCOMPLETE_ADMIN;
}

/**
 * Updates the source of the given url, if its source has changed, 
 * remove the data associated with its old source.
 * 
 * @param int $id
 *   The autocomplete url ID.
 * @param string $source 
 *   The new source of the autocomplete url.
 */
function islandora_autocomplete_url_form_submit_update_source($id, $source) {
  $original_url = islandora_autocomplete_db_get_url($id);
  if ($original_url->source != $source) { // Remove the old source
    module_invoke_all('delete_autocomplete_url', $id);
  }
}

/**
 * Call the submit hook for the source section of the form.
 * 
 * @param array $form
 *   The drupal form.
 * @param array $form_state 
 *   The drupal form state.
 */
function islandora_autocomplete_url_source_form_submit($source, array $form, array &$form_state) {
  $hook = "islandora_autocomplete_url_{$source}_form_submit";
  function_exists($hook) ? call_user_func($hook, $form, $form_state) : array();
}

/**
 * Menu callback; delete a single autocomplete url.
 */
function islandora_autocomplete_delete_confirm(&$form_state, $name) {
  $url = islandora_autocomplete_db_get_url_by_name($name);
  if (!$url) {
    drupal_not_found();
    exit();
  }
  $form['name'] = array('#type' => 'value', '#value' => $name);
  $form['id'] = array('#type' => 'value', '#value' => $url->id);
  $message = t('Are you sure you want to delete the autocomplete url %name?', array('%name' => $name));
  $caption = '<p>' . t('This action cannot be undone.') . '</p>';
  return confirm_form($form, filter_xss($message), MENU_ISLANDORA_AUTOCOMPLETE_ADMIN, filter_xss($caption), t('Delete'));
}

/**
 * Process autocomplete url delete confirm submissions.
 */
function islandora_autocomplete_delete_confirm_submit($form, &$form_state) {
  islandora_autocomplete_db_delete_url($form_state['values']['id']);
  $t_args = array('%name' => $form_state['values']['name']);
  drupal_set_message(t('The autocomplete url %name has been deleted.', $t_args));
  watchdog('menu', 'Deleted autocomplete url %name.', $t_args, WATCHDOG_NOTICE);
  $form_state['redirect'] = MENU_ISLANDORA_AUTOCOMPLETE_ADMIN;
  return;
}

/**
 * Replace the source_form portion of the form when the source changes.
 */
function islandora_autocomplete_url_ahah_source() {
  if (!isset($_REQUEST['form_build_id'])) { // Request is not valid
    header("HTTP/1.0 200 OK", FALSE, 200);
    exit();
  }
  $form_build_id = $_REQUEST['form_build_id'];
  $form_state = array('storage' => NULL, 'submitted' => FALSE, 'post' => $_POST);
  if (!$form = form_get_cache($form_build_id, $form_state)) {
    header("HTTP/1.0 200 OK", FALSE, 200);
    exit();
  }
  $args = $form['#parameters'];
  $form_id = array_shift($args);
  $form_state['post'] = $form['#post'] = $_POST;
  $form['#programmed'] = $form['#redirect'] = FALSE;
  drupal_process_form($form_id, $form, $form_state);
  $form = drupal_rebuild_form($form_id, $form_state, $args, $form_build_id);
  drupal_get_messages('error'); // Clear errors.
  $html = '';
  if (isset($form)) {
    unset($form['source_form']['#prefix'], $form['source_form']['#suffix']);
    $html = drupal_render($form['source_form']);
  }
  $javascript = drupal_add_js(NULL, NULL, 'header');
  $settings = call_user_func_array('array_merge_recursive', $javascript['setting']);
  unset($settings['ahah']['']);
  drupal_json(array(
    'status' => TRUE,
    'data' => theme('status') . $html,
    'settings' => $settings,
  ));
}