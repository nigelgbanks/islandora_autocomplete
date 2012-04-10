<?php

/**
 * @file
 *
 * Functions for administering Autocomplete url's that allow the users to
 * create, update, read, delete Autocomplete URLs.
 */
module_load_include('inc', 'islandora_autocomplete', 'islandora_autocomplete.db');

/**
 * Menu callback; admin menu for autocomplete url's.
 * 
 * @return string
 *   The admin page's HTML.
 */
function islandora_autocomplete_admin() {
  $urls = islandora_autocomplete_db_get_urls();
  $header = array(t('Name'), t('Description'), t('Source'), array('data' => t('Operations'), 'colspan' => '2'));
  $rows = array();
  while ($url = db_fetch_object($urls)) {
    $path = str_replace('%', $url->name, MENU_ISLANDORA_AUTOCOMPLETE_URL);
    $edit_path = str_replace('%', $url->name, MENU_ISLANDORA_AUTOCOMPLETE_EDIT_URL);
    $delete_path = str_replace('%', $url->name, MENU_ISLANDORA_AUTOCOMPLETE_DELETE_URL);
    $row = array(
      l($url->name, $path),
      filter_xss_admin($url->description),
      check_plain($url->source),
    );
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
 * Form definition for adding/updating and autocomplete url.
 * 
 * @param array $form_state
 *   The drupal form state.
 * @param string $name 
 *   The name of Autocomplete URL to update. If not preset this form will be used to create a new url.
 */
function islandora_autocomplete_url_form(array &$form_state, $name = NULL) {
  module_load_include('inc', 'content', 'includes/content.node_form');
  $sources = islandora_autocomplete_get_sources();
  if (empty($sources)) {
    drupal_set_message(t('No sources are defined, please install a module like "islandora autocomplete csv" so that a source can be defined.'), 'error');
    return array();
  }
  $url = isset($name) ? islandora_autocomplete_db_get_url_by_name($name) : FALSE;
  $values = isset($form_state['values']) ? $form_state['values'] : ($url) ? object_as_array($url) : FALSE;
  /**
   * @todo make source use ahah.
   */
  /**
   * @todo handle the submisson of sources section of the form.
   */
  $field = content_fields('filefield');
  return array(
    '#attributes' => array(
      'enctype' => "multipart/form-data"
    ),
    'id' => array(
      '#type' => 'hidden',
      '#value' => isset($values['id']) ? $values['id'] : NULL
    ),
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
      '#description' => t('This is the source from which the autocomplete url will be populated.'),
      '#type' => 'select',
      '#default_value' => isset($values['source']) ? $values['source'] : '',
      '#disabled' => isset($values['id']) ? TRUE : FALSE,
      '#options' => array(
        'csv' => 'CSV',
      )
    ),
    'csv' => array(
      '#title' => 'CSV Upload',
      '#type' => 'filefield_widget',
      '#default_value' => 0,
    ),
    'submit' => array(
      '#type' => 'submit',
      '#value' => t('Submit')
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
 * Either adds a autocomplete url or updates and existing one.
 * 
 * @param array $form
 *   The drupal form.
 * @param array $form_state 
 *   The drupal form state.
 */
function islandora_autocomplete_url_form_submit(array $form, array &$form_state) {
  $values = $form_state['values'];
  if (isset($values['id'])) {
    islandora_autocomplete_db_update_url($values['id'], $values['name'], $values['description'], $values['source']);
  }
  else {
    islandora_autocomplete_db_add_url($values['name'], $values['description'], $values['source']);
  }
  $form_state['redirect'] = MENU_ISLANDORA_AUTOCOMPLETE_ADMIN;
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