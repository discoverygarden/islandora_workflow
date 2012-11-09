<?php
/**
 * @file
 *   This file holds functions used by workflow's batch processes.
 *
 */

/**
 * Start a Drupal batch process to update all children of a collection.
 *
 * @param string|array $collection_pid
 *   Either a string containing the Fedora PID of a collection whose childrens'
 *   POLICY stream we need to update, or an array containing multiple.
 */
function islandora_workflow_update_children($collection_pid) {
  module_load_include('collection.inc', 'islandora_workflow');
  module_load_include('inc', 'islandora_workflow');

  $batch = array(
    'title' => t('Updating XACML Policies'),
    'progress_message' => t('Please wait if many objects are being updated this could take a few minutes.'),
    'operations' => array(),
    'finished' => 'islandora_workflow_batch_finished',
    'file' => __FILE__,
  );

  foreach ((array) $collection_pid as $collection) {
    $members = islandora_workflow_get_all_members_of_collection($collection);
    if ($members) {
      $batch['operations'][] = array(
        'islandora_workflow_batch_function',
        array($collection, $members, FALSE),
      );
    }
  }

  if ($batch['operations']) {
    batch_set($batch);
  }
}

/**
 * Update XACML policies on objects.
 *
 * This is a function that is supposed to be called by drupal_batch.
 * It will update all the members in a
 * collection to either include a XACML policy appropriate for their
 * state, or the child security policy of their
 * parent collection. This function assumes that every pid in the
 * $members array needs to be updated.
 *
 * @param array $collection_pid
 *   PID of the parent collection.
 * @param array $members
 *   Array of members of the collection. It is assumed to have
 *   the structure of the output of
 *   islandora_workflow_get_all_members_of_collection
 * @param bool $delete
 *   Boolean. If this is TRUE all the POLICY streams will be
 *   deleted and the parents CHILD_SECURITY stream will be
 *   added if it exists. If it is FALSE then an appropriate
 *   POLICY stream for workflow state will be added.
 * @param unknown $context
 *   This is defined by drupal_batch.
 */
function islandora_workflow_batch_function($collection_pid, $members, $delete, &$context) {
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');
  module_load_include('inc', 'islandora_xacml_api', 'IslandoraXacml');
  module_load_include('permissions.inc', 'islandora_workflow');
  module_load_include('inc', 'islandora_workflow');

  if (empty($context['sandbox'])) {
    $context['sandbox'] = array();
    $context['sandbox']['progress'] = 0;
    $context['sandbox']['members'] = $members;
    $context['sandbox']['items'] = count($members);
    if (!isset($context['results']['success'])) {
      $context['results']['success'] = array();
    }
    if (!isset($context['results']['fail'])) {
      $context['results']['fail'] = array();
    }
  }

  $data = reset($context['sandbox']['members']);
  $pid = key($context['sandbox']['members']);
  unset($context['sandbox']['members'][$pid]);
  $context['sandbox']['progress']++;

  try {
    $item = new Fedora_Item($pid);

    if (!$delete) {
      $xacml = new IslandoraXacml($pid);
      // Get users and roles.
      $creator = islandora_workflow_get_object_creator($pid);
      $perms = islandora_workflow_get_permission_state($collection_pid, $data['state']);
      $perms['users'][] = $creator;

      $xacml->managementRule->clear();
      $xacml->viewingRule->clear();

      $xacml->managementRule->addRole($perms['roles']);
      $xacml->viewingRule->addRole($perms['roles']);
      $xacml->managementRule->addUser($perms['users']);
      $xacml->viewingRule->addUser($perms['users']);

      $xacml->writeBackToFedora();
    }
    else {
      // XACML is disabled.
      $collection_item = new Fedora_Item($collection_pid);

      if (array_key_exists('CHILD_SECURITY', $collection_items->datastreams)) {
        $child_security = $collection_item->get_datastream_dissemination('CHILD_SECURITY');
        $xacml = new IslandoraXacml($pid, 'POLICY', $child_security);
        $xacml->writeBackToFedora();
      }
      else {
        $xacml = new IslandoraXacml($pid);
        $xacml->managementRule->clear();
        $xacml->viewingRule->clear();
        $xacml->writeBackToFedora();
      }
    }
    $context['results']['success'][] = $pid;
  }
  catch (FedoraAPIRestException $e) {
    $context['results']['fail'][] = $pid;
  }

  $context['finished'] = $context['sandbox']['progress'] / $context['sandbox']['items'];
}

/**
 * Batch completion callback function.
 *
 * This function is called by the drupal batch function when it it completed
 * islandora_workflow_batch_function. This function just prints some info
 * to the screen.
 *
 * @param boolean $success
 *   If the operation was successfull
 *
 * @param array $results
 *   An array containing the results of the batch function.
 *   (Created in islandora_workflow_batch_function).
 * @param unknown $operations
 *   operations
 */
function islandora_workflow_batch_finished($success, $results, $operations) {
  if ($success) {
    drupal_set_message(format_plural(count($results['success']), 'One policy updated.', '@count policies updated.'));
  }
  else {
    $message = t('Finished with an error.');
  }

  if ($results['fail']) {
    foreach ($results['fail'] as $fail) {
      drupal_set_message("Failed to update: $fail", 'error');
    }
  }
}
/**
 * This is a simple wrapper that calls a funciton in normalize_assignee.inc
 * It exists because the Batch API only lets one file be included for batch ops.
 *
 * @param string $object_pid
 *   The PID of the Fedora object
 * @param string $assignee
 *   The Drupal username of the assignee.
 *
 * @return boolean
 *   TRUE if any relationships were purged, FALSE otherwise.
 */
function islandora_workflow_remove_assignee_batch($object_pid, $assignee = NULL) {
  module_load_include('inc', 'islandora_workflow', 'normalize_assignees');
  islandora_workflow_remove_assignee($object_pid, $assignee);
}

/**
 * This is a simple wrapper that calls a funciton in normalize_assignee.inc
 * It exists because the Batch API only lets one file be included for batch ops.
 *
 * @param string $object_pid
 *   the PID of the object.
 */
function islandora_workflow_normalize_assignee_batch($object_pid) {
  module_load_include('inc', 'islandora_workflow', 'normalize_assignees');
  islandora_workflow_normalize_assignee($object_pid, NULL);
}
