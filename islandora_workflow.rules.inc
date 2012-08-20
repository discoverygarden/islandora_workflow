<?php

define(ISLANDORA_RULES_MANAGER, 'islandora_workflow_Manager');
define(ISLANDORA_RULES_EDITOR, 'islandora_workflow_Editor');
define(ISLANDORA_RULES_SUBMITTER, 'islandora_workflow_Submitter');
define(ISLANDORA_RULES_ADMINISTRATOR, 'islandora_workflow_Administrator');

/**
 * Implements hook_rules_action_info().
 */
function islandora_workflow_rules_action_info() {
	return array(
		'islandora_workflow_rules_email_someone' => array(
			'label' => t('Email Someone'),
			'module' => 'Islandora Rules',
			'arguments' => array(
				'object' => array(
					'type' => 'fedora_object',
					'label' => t('The object that has been modified'),
				),
			),
			'eval input' => array('message'),
		)
	);
}

function islandora_workflow_rules_email_recipients($object) {
  module_load_include('inc','fedora_repository','api/fedora_item');

  $recipients = array();
  // get everyone who has the given workflow roles for the parent collection
  $collections = array();
  $parents = $object->get_relationships('isMemberOfCollection');
  foreach($parents as $parent) {
    $collections[] = $parent['object'];
  }
  $options = array(
    'collection' => $collections,
    'permissions' => $settings['roles'],
  );
  $user_permissions = islandora_workflow_get_user_permissions($options);
  // returns an array with the keys [user,collection,permission]
  foreach($user_permissions as $user_permission) {
    $uid = $user_permission['user'];
    $recipients[] = $uid;
  }

  if($settings['notify_creator']) {
    $creator_name = $item->objectProfile->objOwnerId;
    $creator = user_load(array('name' => $creator_name));
    if ($creator) {
      $recipients[]= $creator->uid;
    }
  }
  return $recipients;
}

/**
 * A Rules action to send email to people.
 * @param array $settings
 */
function islandora_workflow_rules_email_someone($object, $settings) {
  $recipients = islandora_workflow_rules_email_recipients($object);
	foreach ($recipients as $recipient) {
		# code...
		// drupal_mail();
    // just log it for now.
    $account = user_load($recipient);
    watchdog('iw_rules','Sending message about %object to %address',
      array('%object' => $settings['object'], '%address' => $account->mail)
    );
	}
}

/**
 * Settings form for the Rules acton to send people email.
 * @param array $settings
 * @param array $form
 */
function islandora_workflow_rules_email_someone_form($settings, &$form) {
	$settings += array(
    'message' => '',
    'roles' => array('admin'),
    'notify_creator' => TRUE,
  );
	$form['settings']['message'] = array(
		'#type' => 'textarea',
		'#title' => t('Message'),
		'#default_value' => $settings['message'],
		'#description' => t('The message to be sent.'),
	);
  $role_options = array(
    ISLANDORA_RULES_ADMINISTRATOR => t('Workflow Administrator'),
    ISLANDORA_RULES_MANAGER => t('Manager'),
    ISLANDORA_RULES_EDITOR => t('Editor'),
  );
  $form['settings']['roles'] = array(
    '#type' => 'select',
    '#multiple' => TRUE,
    '#title' => t('Workflow Role'),
    '#description' => t('Select the roles to be notified.'),
    '#options' => $role_options,
    '#default_value' => $settings['roles'],
  );
  $form['settings']['notify_creator'] = array(
    '#type' => 'checkbox',
    '#title' => t('Notify the object\'s creator'),
    '#default_value' => $settings['notify_creator'],
  );
}

// @TODO validate the form so you can't notify no-one.

/** 
 * Implements hook_rules_condition_info().
 */
function islandora_workflow_rules_condition_info() {
	// none yet ..
}

/**
 * Implement hook_rules_event_info().
 * @return array
 */
function islandora_workflow_rules_event_info() {
	$arguments = array(
		'object_id' => array(
			'label' => t('Object PID'),
			'type' => 'fedora_object',
		),
	);

	return array(
		'islandora_workflow_rules_reject_object' => array(
			'label' => t('After rejecting a Fedora object'),
			'module' => 'Islandora Rules',
			'arguments' => $arguments,
		),
		'islandora_workflow_rules_approve_object' => array(
			'label' => t('After approving a Fedora object'),
			'module' => 'Islandora Rules',	
			'arguments' => $arguments,
		),
		'islandora_workflow_rules_submit_object' => array(
			'label' => t('After submitting a Fedora object'),
			'module' => 'Islandora Rules',
			'arguments' => $arguments,
		),
		'islandora_workflow_rules_assign_object' => array(
			'label' => t('After assigning a Fedora object'),
			'module' => 'Islandora Rules',
			'arguments' => $arguments,
		),
	);
}

/**
 * Implements hook_token_values().
 */
function islandora_workflow_token_values($type, $object = NULL, $options = array()) {
	$values = array();
	if ('fedora_object' == $type) {
		$values['fedora-object-pid'] = $object->pid;
    $values['fedora-object-title'] = $object->objectProfile->objLabel;
    $values['fedora-object-creator'] = $object->objectProfile->objOwnerId;
	}
	return $values;
}

/** 
 * Implements hook_token_list().
 */
function islandora_workflow_token_list($type = 'all') {
  $tokens = array();
  if ('fedora_object' == $type) {
    $tokens['fedora_object']['fedora-object-pid'] = t('The object PID');
    $tokens['fedora_object']['fedora-object-title'] = t('The object\'s title');
    $tokens['fedora_object']['fedora-object-creator'] = t('The object\'s creator');
  }
  return $tokens;
}

/**
 * Implementation of hook_rules_data_type_info().
 */
function islandora_workflow_rules_data_type_info() {
  $data_types = array();
  $data_types['fedora_object'] = array(
    'label' => t('Fedora object'),
    'class' => 'fedora_object_rules_data_type',
    'saveable' => FALSE, // don't think we need to save them just yet
    'identifiable' => TRUE, // we do need to load them though
    'uses_input_form' => FALSE, // don't know, so say false
    'module' => 'Islandora Workflow',
  );
  return $data_types;
}

// specify the data types
class fedora_object_rules_data_type extends rules_data_type {

  function load($pid) {
    module_load_include('inc','fedora_repository','api/fedora_item');
    return new Fedora_Item($pid);
  }

  function get_identifier() {
    $fedora_object = $this->get();
    if ($fedora_object) {
      return $fedora_object->pid;
    }
  }

}

// example usage
// rules_invoke_event('islandora_workflow_rules_approve_object',$object,$relationship);
