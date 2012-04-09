<?php

/**
 * @file
 *
 * Functions to abstract this modules database usage.
 */
define('TABLE_ISLANDORA_AUTOCOMPLETE_CSV', 'islandora_autocomplete_csv');

/**
 * Performs a query and retrieves all the Autocomplete URL's stored in the database.
 *
 * @return
 *   A database query result resource, or FALSE if the query was not
 *   executed correctly.
 */
function islandora_autocomplete_csv_db_get_urls() {
  return db_query('SELECT * FROM {%s}', TABLE_ISLANDORA_AUTOCOMPLETE_CSV);
}

/**
 * Fetches the given Autocomplete URL from the database.
 *
 * @param int $url_id
 *   The unique id for the requested Autocomplete URL.
 *
 * @return
 *   An object representing the given autocomplete url csv source identified by name, or FALSE.
 *   The attributes of this object are the table fields selected by the query.
 */
function islandora_autocomplete_csv_db_get_url($url_id) {
  $result = db_query('SELECT * FROM {%s} WHERE url_id = %d', TABLE_ISLANDORA_AUTOCOMPLETE_CSV, $url_id);
  return $result !== FALSE ? db_fetch_object($result) : FALSE;
}

/**
 * Adds a new Autocomplete URL if one by the same name doesn't already exist.
 *
 * @param string $name
 *   The unique name of the URL to add.
 * @param string $description
 *   A short description of the URL's purpose.
 * @param string $source
 *   The source used to populate the autocomplete URL.
 *
 * @return boolean
 *   TRUE if successful FALSE otherwise.
 */
function islandora_autocomplete_csv_db_add_url($url_id, $filename) {
  if (db_result(db_query('SELECT count(id) FROM {%s} WHERE url_id = %d', ISLANDORA_AUTOCOMPLETE_TABLE, $url_id)) == 0) {
    $object = new stdClass();
    $object->url_id = $url_id;
    $object->filename = $filename;
    return drupal_write_record(TABLE_ISLANDORA_AUTOCOMPLETE_CSV, $object) !== FALSE;
  }
  return FALSE;
}

/**
 * Update the given Autocomplete URL.
 *
 * @param int $id
 *   The unique ID for the record we want to update.
 * @param string $name
 *   The unique name of the URL to add.
 * @param string $description
 *   A short description of the URL's purpose.
 * @param string $source
 *   The source used to populate the autocomplete URL.
 *
 * @return boolean
 *   TRUE if successful FALSE otherwise.
 */
function islandora_autocomplete_csv_db_update_url($id, $url_id, $filename) {
  if (db_result(db_query('SELECT count(id) FROM {%s} WHERE id = %d', TABLE_ISLANDORA_AUTOCOMPLETE_CSV, $id)) == 1) {
    $object = new stdClass();
    $object->id = $id;
    $object->url_id = $url_id;
    $object->filename = $filename;
    return drupal_write_record(TABLE_ISLANDORA_AUTOCOMPLETE_CSV, $object);
  }
  return FALSE;
}

/**
 * Deletes the given Autocomplete URL.
 *
 * @param int $id
 *   The unique id of the URL to delete.
 */
function islandora_autocomplete_csv_db_delete_url($id) {
  db_query('DELETE FROM {%s} WHERE id = "%d"', TABLE_ISLANDORA_AUTOCOMPLETE_CSV, $id);
}