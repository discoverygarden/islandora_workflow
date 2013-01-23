<?php

/**
 * @file
 *   This file holds form related functions for the islandora_workflow module.
 */

/**
 * Defer a drupal_get_form() call.
 *
 * If the Fedora server is unavailable, display an error message; otherwise,
 * works exactly like drupal_get_form().
 *
 * @see drupal_get_form()
 */
function _islandora_workflow_get_form($form_id) {
  module_load_include('inc', 'fedora_repository', 'api/fedora_utils');
  if (!fedora_available()) {
    drupal_set_message(t('The Fedora server is not available'), 'error');
    return drupal_not_found();
  }

  $args = func_get_args();
  return call_user_func_array('drupal_get_form', $args);
}

/**
 * Build forms for the islandora_workflow_work page.
 *
 * @param array $form_state
 *   The current state of the form.
 * @param string $tab
 *   The tab calling the form; 'work_in_progress', 'assigned' or 'unpublished'
 *
 * @return array
 *   $form the array containing the appropriate form
 */
function islandora_workflow_work_form(&$form_state, $tab) {
  module_load_include('permission.inc', 'islandora_workflow');
  module_load_include('inc', 'islandora_workflow');
  module_load_include('collection.inc', 'islandora_workflow');

  if (empty($form_state['storage']['form'])) {
    $form_state['storage']['form'] = 'main';
  }
  switch ($form_state['storage']['form']) {
    case 'main':
      return islandora_workflow_work_form_main($form_state, $tab);
      break;

    case 'delete':
      return islandora_workflow_work_form_delete($form_state);
      break;

    case 'reject':
      return islandora_workflow_work_form_reject($form_state);
      break;

    default:
      drupal_not_found();
      exit;
  }
}

/**
 * Form definition.
 *
 * @param array $form_state
 *   The current state of the form.
 * @param string $tab
 *   The active tab on the workflow page.
 *
 * @return array
 *   A form definition.
 */
function islandora_workflow_work_form_main(&$form_state, $tab) {
  module_load_include('collection.inc', 'islandora_workflow');
  module_load_include('inc', 'islandora_workflow', 'islandora_workflow');
  module_load_include('inc', 'islandora_workflow', 'normalize_assignees');
  module_load_include('inc', 'fedora_repository', 'PagerSetup');

  global $user;

  $items_per_page = intval(variable_get('islandora_workflow_items_per_page', 10));

  $items = array();

  $users_collections = islandora_workflow_get_users_collections('all');
  /* Populate an array of all users in relevant collections,
   *  to be used in the assign-to-user drop down. */
  $users_collections_members = islandora_workflow_get_all_users_with_collection_permissions(array_keys($users_collections));

  $form = array(
    'workflow' => array(
      '#type' => 'fieldset',
      '#title' => t('Workflow'),
      '#collapsible' => FALSE,
      '#collapsed' => FALSE,
      '#tree' => TRUE,
      '#description' => t('Perform work on objects you have access to'),
      'elements' => array(
        '#theme' => 'workflow_table',
      ),
    ),
  );

  $elements = array();
  // Get items.
  switch ($tab) {
    case 'work_in_progress':
      $elements = islandora_workflow_assignment_by_user($user);
      $elements += _islandora_workflow_get_created_items($user);

      // Show everything created by the user except for submitters.
      // Show everything created by and in a created state to submitters.
      $manager_collections = islandora_workflow_get_users_collections('islandora_workflow_Manager');
      $editor_collections = islandora_workflow_get_users_collections('islandora_workflow_Editor');
      $editor_collections = array_diff_key($editor_collections, $manager_collections);
      $submitter_collections = islandora_workflow_get_users_collections('islandora_workflow_Submitter');
      $submitter_collections = array_diff_key($submitter_collections, $manager_collections, $editor_collections);

      $submitter_objects_to_ommit = array();

      // Assignees show things they own that are in created/rejected state only.
      // Double use of array_diff_key on $collection_members_in_progress.
      foreach ($submitter_collections as $collection_pid => $attributes) {
        $collection_members = islandora_workflow_tracked_collection_members($collection_pid);
        $collection_members_in_progress = array_filter($collection_members, '_islandora_workflow_in_progress_by_submitter');
        $submitter_objects_to_ommit += array_diff_key($collection_members, $collection_members_in_progress);
      }

      // This lets editors see the objects created by anonymous users.
      foreach ($editor_collections as $collection_pid => $attributes) {
        $members = islandora_workflow_tracked_collection_members($collection_pid);
        $elements += array_filter($members, '_islandora_workflow_in_progress_by_editor');
      }

      $elements = array_diff_key($elements, $submitter_objects_to_ommit);
      break;

    case 'assigned':
      $elements = islandora_workflow_assignment_by_user($user);
      break;

    case 'unpublished':
      $manager_collections = islandora_workflow_get_users_collections('islandora_workflow_Manager');
      $editor_collections = islandora_workflow_get_users_collections('islandora_workflow_Editor');
      $editor_collections = array_diff_key($editor_collections, $manager_collections);
      $submitter_collections = islandora_workflow_get_users_collections('islandora_workflow_Submitter');
      $submitter_collections = array_diff_key($submitter_collections, $manager_collections, $editor_collections);

      foreach ($manager_collections as $collection_pid => $attributes) {
        $members = islandora_workflow_tracked_collection_members($collection_pid);
        $elements += array_filter($members, '_islandora_workflow_actionable_by_manager');
      }
      foreach ($editor_collections as $collection_pid => $attributes) {
        $members = islandora_workflow_tracked_collection_members($collection_pid);
        $elements += array_filter($members, '_islandora_workflow_actionable_by_editor');
      }
      foreach ($submitter_collections as $collection_pid => $attributes) {
        $members = islandora_workflow_tracked_collection_members($collection_pid);
        $elements += array_filter($members, '_islandora_workflow_actionable_by_submitter');
      }
      break;

    default:
      drupal_not_found();
      exit;
      break;
  }

  // Setup table headers.
  $headers = array(
    array(
      'data' => ' ',
    ),
    array(
      'data' => t('Title'),
      'field' => 'label',
    ),
    array(
      'data' => t('Collection'),
      'field' => 'collection_label',
    ),
    array(
      'data' => t('Status'),
      'field' => 'state',
    ),
    array(
      'data' => t('Object Created'),
      'field' => 'created_date',
    ),
    array(
      'data' => t('Last Workflow Movement'),
      'field' => 'islandora_workflow_modified',
      'sort' => 'desc',
    ),
    array(
      'data' => t('Notes'),
    ),
    array(
      'data' => t('Assignee'),
      'field' => 'assignee',
    ),
  );

  module_load_include('sort.inc', 'islandora_workflow');

  $sort_field = tablesort_get_order($headers);
  $sort_order = tablesort_get_sort($headers);
  $sort_function = "islandora_workflow_sort_{$sort_field['sql']}_{$sort_order}";

  // We do the sort PHP side, hopefully this will scale.
  uasort($elements, $sort_function);

  $form_state['storage']['elements'] = $elements;
  $form_state['storage']['headers'] = $headers;

  // Add to form.
  $form['workflow']['elements']['header'] = array(
    '#value' => $headers,
  );

  // Setup the pager.
  $pager_id = 0;
  $total_items = count($elements);
  $page = fedora_repository_setup_pager($pager_id, $items_per_page, $total_items);

  $offset = $page * $items_per_page;

  if ($total_items > $items_per_page) {
    $offset = $page * $items_per_page;
    $elements = array_slice($elements, $offset, $items_per_page);
  }

  $form['workflow']['pager'] = array(
    '#type' => 'markup',
    '#weight' => 1000,
    '#value' => theme('pager', array(), $items_per_page, $pager_id),
  );

  foreach ($elements as $object_pid => $attributes) {
    // Get possible assignees for the object.
    $possible_assignees = islandora_workflow_get_possible_assignees(array($object_pid), array(), $format = TRUE);
    $possible_assignees = $possible_assignees[$object_pid];

    // Populate the selector and assignee drop down.
    $form['workflow']['elements'][$object_pid] = array(
      'Selecter' => array(
        '#type' => 'checkbox',
        '#title' => '',
      ),
      'Assign' => array(
        '#type' => 'select',
        '#default_value' => isset($attributes['assignee']) ? $attributes['assignee'] : '',
        '#options' => $possible_assignees[$attributes['state']] ? $possible_assignees[$attributes['state']] : array(),
      ),
      'table_info' => array('#value' => $attributes),
    );
  }

  // Perform additional processing (like filling in possible assignees).

    $form['Submit'] = array(
      '#type' => 'submit',
      '#weight' => '1',
      '#value' => t('Submit'),
    );

  $form['Approve'] = array(
    '#type' => 'submit',
    '#weight' => '2',
    '#value' => t('Approve'),
    '#access' => _islandora_workflow_reject_access(),
  );
  $form['Publish'] = array(
    '#type' => 'submit',
    '#weight' => '3',
    '#value' => t('Publish'),
    '#access' => _islandora_workflow_publish_access(),
  );
  $form['Reject'] = array(
    '#type' => 'submit',
    '#weight' => '4',
    '#value' => t('Reject...'),
    '#access' => _islandora_workflow_reject_access(),
  );
  $form['Claim'] = array(
    '#type' => 'submit',
    '#weight' => '5',
    '#value' => t('Claim'),
    '#access' => ($tab != 'assigned'),
  );
  $form['Assign'] = array(
    '#type' => 'submit',
    '#weight' => '6',
    '#value' => t('Assign'),
  );
  $form['Delete'] = array(
    '#type' => 'submit',
    '#weight' => '7',
    '#value' => t('Delete...'),
  );

  $form['#submit'][] = 'islandora_workflow_work_form_submit';
  $form['#validate'][] = 'islandora_workflow_work_form_validate';
  return $form;
}

/**
 * Form definition.
 *
 * Return the second step of the form, to be displayed if
 * the user clicks 'Reject...' after selecting one or more objects in step one.
 * Give the user the option to re-assign each selected object before rejection.
 *
 * @param array $form_state
 *   The existing form state
 *
 * @return array
 *   The form definition
 */
function islandora_workflow_work_form_reject(&$form_state) {
  module_load_include('inc', 'islandora_workflow', 'normalize_assignees');
  $form = array();

  $objects = $form_state['storage']['selected_objects'];
  $elements = $form_state['storage']['elements'];
  $headers = $form_state['storage']['headers'];

  foreach ($headers as $key => $header) {
    unset($headers[$key]['field']);
  }


  $form['assignees'] = array(
    '#type' => 'fieldset',
    '#title' => t('Assign Rejected Objects'),
    '#description' => t('You may assign rejected objects to a user with
      suitable permissions. If you do not assign the rejected object to
      a particular user, the rejected object will still appear under
      "Unpublished Objects" for all editors and managers
      in this collection.'),
    '#tree' => TRUE,
  );

  foreach ($objects as $object) {

    $possible_assignees = islandora_workflow_get_possible_assignees(array($object), array(), $format = TRUE);
    $options = $possible_assignees[$object]['rejected'];

    $form['assignees'][$object] = array(
      '#type' => 'fieldset',
      '#title' => $elements[$object]['label'],
    );

    $form['assignees'][$object]['assignee'] = array(
      '#type' => 'select',
      '#default_value' => isset($elements[$object]['assignee']) ? $elements[$object]['assignee'] : '',
      '#options' => $options ? $options : array(),
    );
  }

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Reject and Assign'),
  );

  $form['cancel'] = array(
    '#type' => 'submit',
    '#value' => t('Cancel'),
  );

  $form['#submit'] = array('islandora_workflow_work_form_reject_submit');
  return $form;
}

/**
 * Form definition.
 *
 * Return the second step of the form, to be displayed if the user
 * clicks 'delete'.
 *
 * @param array $form_state
 *   The existing form state
 *
 * @return array
 *   The form definition
 */
function islandora_workflow_work_form_delete(&$form_state) {
  $form = array();

  drupal_set_message(t('Deleting items removes them from workflow and from the active repository. This action is not reversible. Are you sure you want to continue?'), 'warning');
  $form['delete'] = array(
    '#type' => 'submit',
    '#value' => t('Delete Objects'),
  );
  $form['cancel'] = array(
    '#type' => 'submit',
    '#value' => t('Cancel'),
  );

  $form['#submit'] = array('islandora_workflow_work_form_delete_submit');
  return $form;
}

/**
 * Delete objects and redirect if the user confirmed.
 *
 * @param array $form
 *   The form
 * @param array $form_state
 *   The existing form state
 */
function islandora_workflow_work_form_delete_submit($form, &$form_state) {
  if ($form_state['clicked_button']['#value'] == t('Delete Objects')) {
    $selected_objects = $form_state['storage']['selected_objects'];
    foreach ($selected_objects as $object_id) {
      islandora_workflow_delete_object($object_id);
    }
  }

  $form['#redirect'] = array('islandora_workflow');

  // Get rid of storage so the redirect can work.
  unset($form_state['storage']);
}

/**
 * Form submit handler.
 *
 * Reject (and optionally assign) one or more objects.
 *
 * @param array $form
 *   The form
 * @param array $form_state
 *   The existing form state
 */
function islandora_workflow_work_form_reject_submit($form, &$form_state) {
  module_load_include('inc', 'islandora_workflow');
  module_load_include('inc', 'islandora_workflow', 'normalize_assignees');

  if ($form_state['clicked_button']['#value'] == t('Reject and Assign')) {
    foreach ($form_state['values']['assignees'] as $pid => $values) {
      islandora_workflow_reject_object($pid);
      if (empty($values['assignee'])) {
        islandora_workflow_remove_assignee($pid);
      }
      else {
        islandora_workflow_assign_object($pid, $values['assignee']);
      }
    }
  }

  $form['#redirect'] = array('islandora_workflow');
  // Get rid of storage so the redirect can work.
  unset($form_state['storage']);
}

/**
 * Form submit handler.
 *
 * This function handles all of the possible submits
 * from the workflow work form (overview and workflow)
 *
 * @param array $form
 *   Holds all the data on the state  of the form submitted
 * @param array $form_state
 *   An easier to use version of $work_form_form
 */
function islandora_workflow_work_form_submit($form, &$form_state) {
  module_load_include('inc', 'islandora_workflow');
  module_load_include('inc', 'islandora_workflow', 'normalize_assignees');

  $clicked_button = $form_state['clicked_button']['#value'];
  $selected_object_ids = array();
  $assignees = array();
  $assignee_indexes = array();

  // Get selected objects.
  foreach ($form_state['values']['workflow']['elements'] as $object_id => $values) {
    if ($values['Selecter'] == 1) {
      $selected_object_ids[] = $object_id;
      // Get selected assignees.
      $assignees[$object_id] = $values['Assign'];
    }
  }

  // Handle buttons.
  if ($clicked_button == 'Submit') {
    foreach ($selected_object_ids as $object_id) {
      islandora_workflow_submit_object($object_id);
    }
  }
  elseif ($clicked_button == 'Delete...') {
    $form_state['storage']['form'] = 'delete';
    $form_state['storage']['selected_objects'] = $selected_object_ids;
    $form_state['rebuild'] = TRUE;
  }
  elseif ($clicked_button == 'Reject...') {
    $form_state['storage']['form'] = 'reject';
    $form_state['storage']['selected_objects'] = $selected_object_ids;
    $form_state['rebuild'] = TRUE;
  }
  elseif ($clicked_button == 'Approve') {
    foreach ($selected_object_ids as $object_id) {
      islandora_workflow_approve_object($object_id);
    }
  }
  elseif ($clicked_button == 'Publish') {
    foreach ($selected_object_ids as $object_id) {
      islandora_workflow_publish_object($object_id);
    }
  }
  elseif ($clicked_button == 'Claim') {
    foreach ($selected_object_ids as $object_id) {
      islandora_workflow_claim_object($object_id);
    }
  }
  elseif ($clicked_button == 'Assign') {
    foreach ($selected_object_ids as $object_id) {
      if (empty($assignees[$object_id])) {
        islandora_workflow_remove_assignee($object_id);
      }
      else {
        islandora_workflow_assign_object($object_id, $assignees[$object_id]);
      }
    }
  }
}

/**
 * Form validation function.
 *
 * After the first step of the form has been submitted, this function checks
 * that at least one checkbox (for an object) has been checked.
 * If not the form submission would be meaningless.
 *
 * @param array $form
 *   The form.
 * @param array $form_state
 *   The form state.
 */
function islandora_workflow_work_form_validate($form, &$form_state) {
  $clicked_button = $form_state['clicked_button']['#value'];
  switch ($clicked_button) {
    case 'Submit':
    case 'Approve':
    case 'Assign':
    case 'Publish':
    case 'Reject...':
    case 'Claim':
    case 'Delete':
      $selected_object_ids = array();
      // Get selected objects.
      $elements = $form_state['values']['workflow']['elements'];
      foreach ($elements as $object_id => $values) {
        if ($values['Selecter'] == 1) {
          $selected_object_ids[] = $object_id;
        }
      }
      if (empty($selected_object_ids)) {
        form_set_error('workflow][elements',
            'No objects selected.'
        );
      }
      break;

    default:
      break;
  }
}
