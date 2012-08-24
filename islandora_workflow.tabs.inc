<?php

/**
 * Implementation of hook_islandora_tabs_alter().
 *
 * @param &$tabs array
 *   The array of tabs/tabset to alter.
 * @param $params array
 *   An associative array containing the parameters with which the original
 *   hook was called.
 * @see fedora_repository_get_items()
 */
function islandora_workflow_islandora_tabs_alter(&$tabs, $params) {

  module_load_include('inc','fedora_repository','ObjectHelper');
  module_load_include('inc','islandora_workflow','islandora_workflow');
  module_load_include('inc','islandora_workflow','islandora_workflow.permissions');
  
  $pid = $params['pid'];
  $content_models = $params['content_models'];
  $parent = islandora_workflow_get_object_parent($pid);
  $permission = islandora_workflow_user_object_permissions_check($pid);
  $objectHelper = new ObjectHelper();

  if (module_exists('islandora_collection_manager')) {
    // restrict the 'Manage This Collection' tab to managers and above
    $management_access = ('islandora_workflow_Manager' == $permission);
    if (!$management_access) {
      unset($tabs['add_collection_tab']);
    }
  }

  // restrict the 'Add' tab on collection pages to Submitters and above
  $create_access = !empty($permission);
  if(!$create_access) {
    unset($tabs['add_tab']);
  }

  // restrict object details/edit to:
  if (array_key_exists('fedora_object_details', $tabs)) {
    $is_a_collection = FALSE;
    foreach($content_models as $content_model) {
      if ('islandora:collectionCModel' == $content_model->pid) {
        $is_a_collection = TRUE;
        break;
      }
    }
    if ($is_a_collection) {
      // managers and above
      $update_access = ('islandora_workflow_Manager' == $permission);
    }
    else {
      // submitters and above
      $update_access = !empty($permission);
    }
    if (!$update_access) {
      unset($tabs['fedora_object_details']['tabset']['view']['purge']);
      unset($tabs['fedora_object_details']['tabset']['edit']);
    }
  }

}

