<?php

/**
 * @file
 * Implement the module theme functions.
 */

 /**
  * Theme preprocess function.
  *
  * This function handles making variables available to the theme template for
  * the islandora_workflow_collections page
  *
  * @param array $vars
  *   the list of variables (including the form) to send to the template
  *
  * @return array
  *   $vars the modified list of variables to send to the template
  */
function islandora_workflow_preprocess_workflow_collections(&$vars) {

  module_load_include('inc', 'islandora_workflow');
  module_load_include('inc', 'islandora_workflow', 'islandora_workflow.collection');

  $collections = islandora_workflow_get_collections();

  $vars['collections'] = $collections;
  $vars['list']['submit'] = drupal_render($vars['list']['submit']);
  $active_collection_pid = $_SESSION['workflow_permissions_page']['active_collection'];
  $vars['list']['active_collection_pid'] = $active_collection_pid;
  $vars['list']['active_collection_label'] = islandora_workflow_get_object_label($active_collection_pid);
  // Tender each form element.
  foreach ($collections as $collection_pid => $collection_information) {
    $vars['list']['collection_checkboxes'][$collection_pid] = drupal_render($vars['list'][$collection_pid]);
  }
}

 /**
  * Theme preprocess function.
  *
  * This function handles making variables available to the theme template for
  * the islandora_workflow_collection_permissions page (user tab)
  *
  * @param array $vars
  *   the list of variables (including the form) to send to the template
  *
  * @return array
  *   $vars the modified list of variables to send to the template
  */
function islandora_workflow_preprocess_permissions_user_table(&$vars) {
  module_load_include('inc', 'islandora_workflow', 'islandora_workflow.permissions');

  $vars['users'] = islandora_workflow_get_users_with_module_permissions();
  $vars['list']['collection_selector'] = drupal_render($vars['list']['collection_selector']);
  $vars['list']['collection_submit'] = drupal_render($vars['list']['collection_submit']);
  $active_collection_pid = $_SESSION['workflow_permissions_page']['active_collection'];
  $vars['list']['active_collection_pid'] = $active_collection_pid;
  $vars['list']['active_collection_label'] = islandora_workflow_get_object_label($active_collection_pid);


  // Render each form element.
  foreach ($vars['users'] as $user_id => $user_name) {
    $vars['list'][$user_id][$active_collection_pid] = drupal_render($vars['list'][$user_id][$active_collection_pid]);
  }
}

/**
 * Theme preprocess function.
 *
 * This function handles making variables available to the theme template for
 * the islandora_workflow_collection_permissions page (role tab)
 *
 * @param array $vars
 *   the list of variables (including the form) to send to the template
 *
 * @return array
 *   $vars the modified list of variables to send to the template
 */
function islandora_workflow_preprocess_permissions_role_table(&$vars) {
  module_load_include('inc', 'islandora_workflow', 'islandora_workflow.permissions');

  $vars['roles'] = islandora_workflow_get_roles_with_module_permissions();
  $vars['list']['collection_selector'] = drupal_render($vars['list']['collection_selector']);
  $vars['list']['collection_submit'] = drupal_render($vars['list']['collection_submit']);

  $active_collection_pid = $_SESSION['workflow_permissions_page']['active_collection'];
  $vars['list']['active_collection_pid'] = $active_collection_pid;
  $vars['list']['active_collection_label'] = islandora_workflow_get_object_label($active_collection_pid);

  // Render each form element.
  foreach ($vars['roles'] as $role_id => $role_name) {
    $vars['list'][$role_id][$active_collection_pid] = drupal_render($vars['list'][$role_id][$active_collection_pid]);
  }
}

/**
 * Theme preprocess function.
 *
 * This function handles making variables available to the theme template for
 * the workflow tabs of the 'work' page
 *
 * @param array $vars
 *   the list of variables (including the form) to send to the template
 *
 * @return array
 *   $vars the modified list of variables to send to the template
 */
function islandora_workflow_preprocess_workflow_table(&$vars) {
  // Drupal made me do it.

  module_load_include('inc', 'islandora_workflow', 'islandora_workflow');
  module_load_include('inc', 'fedora_repository', 'api/fedora_collection');

  $vars['collections'] = islandora_workflow_get_users_collections('all');
  foreach ($vars['collections'] as $collection_id => $collection_information) {
    $collection_name = $collection_information['label'];
    $collection_members = islandora_workflow_get_all_members_of_collection($collection_id);
    if ($collection_members) {
      /* Makes it useful for theme as a list off all the
       * members of all the collections.*/
      $vars['collections'][$collection_id] = array($collection_name => $collection_members);

      foreach ($collection_members as $member => $member_attributes) {
        /* The isset is here so that only populated
         * items are rendered into HTML for display.*/
        if (isset($vars['list'][$collection_id][$member])) {

          $member_workflow_state = $member_attributes['state'];
          // Make sure workflow data is present too.
          if ($member_workflow_state) {
            // Current workflow state.
            $vars['list'][$collection_id][$member]['state'] = $member_workflow_state;

            // Render each object element.
            $vars['list'][$collection_id][$member]['Selecter'] = drupal_render($vars['list'][$collection_id][$member]['Selecter']);
            $vars['list'][$collection_id][$member]['Assign'] = drupal_render($vars['list'][$collection_id][$member]['Assign']);

            // Get the dates and provide them.
            $vars['list'][$collection_id][$member]['when_created'] = format_date(strtotime($member_attributes['created_date']), $type = 'small');
            $vars['list'][$collection_id][$member]['last_workflow_progression'] = format_date(strtotime($member_attributes['islandora_workflow_modified']), $type = 'small');

            // If there is a note.
            $member_note_subject = $member_attributes['subject'];
            if ($member_note_subject) {
              $vars['list'][$collection_id][$member]['note_subject'] = l($member_note_subject, 'islandora_workflow_edit_note', array('query' => 'object=' . $member));
            }
            // If there is no note.
            else {
              $vars['list'][$collection_id][$member]['note_subject'] = l(t('Create Note'), 'islandora_workflow_edit_note', array('query' => 'object=' . $member));
            }

            $member_label = $member_attributes['label'];
            // If there is a label.
            if ($member_label) {
              $vars['list'][$collection_id][$member]['object'] = l($member_label, 'fedora/repository/' . $member);
            }
            else {
              // No label, use the pid.
              $vars['list'][$collection_id][$member]['object'] = l($member, 'fedora/repository/' . $member);
            }
          }
        }
      }
    }
    else {
      // Remove the collection in it has no members.
      unset($vars['collections'][$collection_id]);
    }
  }
}