<?php
/**
 * @file
 *   This file should hold object level helper functions for the workflow module.
 */

/**
 * This code was being used allot in the module for feedback to the user
 * so it has been pulled out into a utility function.  It will send a
 * drupal_set_message with the object's label, PID and the message supplied.
 *
 * @param string $PID
 *   The Fedora PID of the object to write the Drupal message about.
 * @param string $message
 *   Please include the appropriate ending punctuation.
 * @param string $label
 *   Defaults to NULL, if it is not set the function will try and get the
 *   object's label.
 */
function islandora_workflow_DSM_about_object($PID, $message, $label = NULL) {
  // Set $label if it is not supplied.
  if (is_null($label)) {
    $label = islandora_workflow_get_object_label($PID);
  }

  // Send the drupal_set_message
  drupal_set_message(
    t('The object <em>%label</em> (%PID) %message',
      array(
        '%label' => $label,
        '%PID' => $PID,
        '%message' => $message,
      )
    )
  );
}

/**
 * Get the label for a Fedora object with a given PID.
 *
 * @param string $object_id
 *   The object to get the label of
 *
 * @return string $label
 *   The label of the object specified.
 *
 * @return boolean
 *   FALSE if the Fedora item does not exist.
 */
function islandora_workflow_get_object_label($object_id) {
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');

  $item = new Fedora_Item($object_id);
  if (!$item->exists()) {
    return FALSE;
  }
  if (!isset($item->objectProfile->objLabel)) {
    return '';
  }
  return $item->objectProfile->objLabel;
}
