<?php

/**
 * @file
 *
 * Callback for querying a autocomplete url.
 */

/**
 * The primary callback that returns the requested autocomplete values.
 *
 * @param string $name
 *   The unique name of the autocomplete url to use.
 *
 * @param string $value
 *   The search query for the autocomplete.
 */
function islandora_autocomplete_query($name, $value) {
  module_load_include('inc', 'islandora_autocomplete', 'islandora_autocomplete.db');
  $url = islandora_autocomplete_db_get_url_by_name($name);
  if ($url) {
    $query_hook = "islandora_autocomplete_url_query_{$url->source}";
    $results = function_exists($query_hook) ? call_user_func($query_hook, $url->id, $value) : FALSE;
    if ($results) {
      return drupal_json(array('status' => TRUE, 'data' => $results));
    }
    else {
      return drupal_json(array('status' => FALSE));
    }
  }
}
