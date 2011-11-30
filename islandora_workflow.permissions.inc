<?php
/**
 * @file This file contains all of the helper functions dealing with Permissions for
 * Islandora workflow.
 */

/**
 * This function queries the database to get information about the islandora workflow users table.
 *
 * @param $options
 *   An associative array of options if no options are passed in then the function will
 *   query for everything. The array contains the following elements:
 *   - 'user': (int) the drupal uid to query for.
 *   - 'collection': (string) the pid of the collection.
 *   - 'permissions': (array) an array of permission strings.
 *   - 'info': (boolean) if TRUE a join is done to return the username as well as drupal user id.
 *
 * @return
 *   indexed array containing all of the results returned. each individual result is a associative array
 *   with the keys: user, collection, permission and optionally name.
 */
function islandora_workflow_get_user_permissions ( array $options = array() ) {
  $user = isset($options['user']) ? $options['user'] : NULL;
  $collection = isset($options['collection']) ? $options['collection'] : NULL;
  $permissions = isset($options['permissions']) ? $options['permissions'] : NULL;
  $userinfo = isset($options['info']) ? $options['info'] : FALSE;

  $and = FALSE;
  $where = FALSE;

  $results = array();
  $args = array();

  if($userinfo) {
    $query = 'SELECT user, name, collection, permission FROM {islandora_workflow_user_permissions} iw, ' .
    '{users} u WHERE iw.user = u.uid';
    $and = TRUE;
    $where = TRUE;
  }
  else {
    $query = 'SELECT user, collection, permission FROM {islandora_workflow_user_permissions} iw';
  }

  if($user) {
    if(!$where) {
      $query .= ' WHERE';
      $where = TRUE;
    }
    if($and)
      $query .= ' AND';
    else
      $and = TRUE;
    $query .= " iw.user = %d";
    $args[] = $user;
  }

  if($collection) {
    if(!$where) {
      $query .= ' WHERE';
      $where = TRUE;
    }
    if($and)
      $query .= ' AND';
    else
      $and = TRUE;
    $query .= " iw.collection = '%s'";
    $args[] = $collection;
  }

  if($permissions) {
    if(!$where) {
      $query .= ' WHERE';
      $where = TRUE;
    }
    if($and)
      $query .= ' AND';
    else
      $and = TRUE;

    $or = FALSE;
    $query .= ' (';
    foreach ($permissions as $permission) {
      if($or)
        $query .= ' OR';
      $query .= " iw.permission = '%s'";
      $args[] = $permission;
      $or = TRUE;
    }
    $query .= ') ';
  }

  array_unshift($args, $query);

  $resource = call_user_func_array('db_query', $args);

  //build return array
  while ($row=db_fetch_array($resource)) {
    $results[] = $row;
  }

  return $results;
}

/**
 * This function queries the database to get information about the islandora workflow roles table.
 *
 * @param $options
 *   An associative array of options if no options are passed in then the function will
 *   query for everything. The array contains the following elements:
 *   - 'roles': (array) an array of drupal rid to query for.
 *   - 'collection': (string) the pid of the collection.
 *   - 'permissions': (array) an array of permissions strings.
 *   - 'info': (boolean) if TRUE a join is done to return the role name as well as drupal role id.
 *
 * @return
 *   indexed array containing all of the results returned. each individual result is a associative array
 *   with the keys: role, collection, permission and optionally name.
 */
function islandora_workflow_get_role_permissions( array $options = array() ) {
  $roles = isset($options['roles']) ? $options['roles'] : array();
  $collection = isset($options['collection']) ? $options['collection'] : NULL;
  $permissions = isset($options['permissions']) ? $options['permissions'] : NULL;
  $roleinfo = isset($options['info']) ? $options['info'] : FALSE;

  $and = FALSE;
  $where = FALSE;

  $results = array();
  $args = array();

  if($roleinfo) {
    $query = 'SELECT role, name, collection, permission FROM {islandora_workflow_role_permissions} iw, ' .
    '{role} r WHERE iw.role = r.rid';
    $and = TRUE;
    $where = TRUE;
  }
  else {
    $query = 'SELECT role, collection, permission FROM {islandora_workflow_role_permissions} iw';
  }

  if($roles) {
    if(!$where) {
      $query .= ' WHERE';
      $where = TRUE;
    }
    if($and)
      $query .= ' AND';
    else
      $and = TRUE;
    $or = FALSE;
    $query .= ' (';
    foreach ($roles as $role) {
      if($or)
        $query .= ' OR';
      $query .= " iw.role = %d";
      $args[] = $role;
      $or = TRUE;
    }
    $query .= ' )';
  }

  if($collection) {
    if(!$where) {
      $query .= ' WHERE';
      $where = TRUE;
    }
    if($and)
      $query .= ' AND';
    else
      $and = TRUE;
    $query .= " iw.collection = '%s'";
    $args[] = $collection;
  }

  if($permissions) {
    if(!$where) {
      $query .= ' WHERE';
      $where = TRUE;
    }
    if($and)
      $query .= ' AND';
    else
      $and = TRUE;

    $or = FALSE;
    $query .= '(';
    foreach ($permissions as $permission) {
      if($or)
        $query .= ' OR';
      $query .= " iw.permission = '%s'";
      $args[] = $permission;
      $or = TRUE;
    }
    $query .= ')';
  }

  array_unshift($args, $query);

  $resource = call_user_func_array('db_query', $args);

  //build return array
  while ($row=db_fetch_array($resource)) {
    $results[] = $row;
  }

  return $results;
}

/**
 * This will get all of the users and roles that should have access to a particular collection given a state.
 * It is used when making XACML policies.
 *
 * @param $collection
 *   Collection PID.
 * @param $state
 *   Workflow state.
 * @return array()
 *   An associtiative array containing 'roles' and 'users'. Each of those is an indexed array containing the
 *   usernames and rolenames.
 */
function islandora_workflow_get_permission_state($collection, $state) {
  $permission = array();
  $users = array();
  $roles = array();

  switch($state) {
    case 'created':
    case 'rejected':
      $permission[] = 'islandora_workflow_Submitter';
    case 'submitted':
      $permission[] = 'islandora_workflow_Editor';
    case 'approved':
      $permission[] = 'islandora_workflow_Manager';
    default:
      $permission[] = 'islandora_workflow_Administrator';
  }

  $user_results = islandora_workflow_get_user_permissions(array('collection' => $collection, 'info' => TRUE, 'permissions' => $permission));
  $role_results = islandora_workflow_get_role_permissions(array('collection' => $collection, 'info' => TRUE, 'permissions' => $permission));

  foreach ($role_results as $role) {
    $roles[] = $role['name'];
  }

  foreach ($user_results as $user) {
    $users[] = $user['name'];
  }

  $users = array_merge(user_access_list('islandora_workflow_Administrator'), $users);

  return array('users' => $users, 'roles' => $roles);
}

/**
 * This function query's drupal's DB for all the collection permission entries by role
 * used when displaying the current state for the permission page.
 *
 * @return array $perm_list
 *   list of permissions that roles have with the form $perm_list[$role_id][$collection_id]=perms
 */
function islandora_workflow_get_all_role_permissions() {
  $perm_list=array();

  $results = islandora_workflow_get_role_permissions();

  //build return array
  foreach($results as $row) {
    $perm_list[$row['role']][$row['collection']]=$row['permission'];
  }

  return $perm_list;
}


/**
 * This function query's drupal's DB for all the collection permission entries for users
 * Used when displaying the current state for the permission page
 * @return array $perm_list
 *   list of all permisions that users have  with the form  $perm_list[$user_id][$collection_id][1...4]=perms
 */
function islandora_workflow_get_all_user_permissions() {
  $perm_list=array();

  $results = islandora_workflow_get_user_permissions();

  //build return array
  foreach($results as $row) {
    $perm_list[$row['user']][$row['collection']]=$row['permission'];
  }

  return $perm_list;
}

/**
 * This function will get all the drupal users who have the indicated or any level of access
 * if none is indicated to the indicated collections
 * @param array $collections
 *   the collection(s) to get the permissions on
 * @param string $permission_level
 *   Defaults to NULL, the permission level users to return
 * @return array $users
 *   an associative array of the indicated collections and all the drupal user names that
 *   have permissions on them
 */
function islandora_workflow_get_all_users_with_collection_permissions($collections, $permission_level=NULL) {
  //init
  //all user perms
  $user_permissions=islandora_workflow_get_all_user_permissions();
  //all role perms
  $role_permissions=islandora_workflow_get_all_role_permissions();
  $users_via_role=array();
  //all users that may be in collections
  $possible_users=islandora_workflow_get_users_with_module_permissions();
  $users=array();
  $admins=user_access_list('islandora_workflow_Administrator');

  //make sure those users with admin rights show up for every collection/perm level
  foreach ($collections as $collection_id) {
    $users[$collection_id]=array();
    //islandora_workflow_Administrator
    foreach ($admins as $user_id => $user_name) {
      $users[$collection_id][]=$user_name;
    }
  }

  //from user perms
  foreach ($user_permissions as $user_id => $collection_permission) {
    foreach ($collection_permission as $collection_id => $permission) {
      if (($permission_level==NULL || $permission==$permission_level) && in_array($collection_id, $collections)) {
        $user_name=user_load($user_id)->name;
          //init the collection if it needs to be
        if (!isset($users[$collection_id])) {
          $users[$collection_id]=array();
        }
        if (!in_array($user_name, $users[$collection_id])) {
          //add the user to the users array for the collection if they aren't in it
          $users[$collection_id][]=$user_name;
        }
      }
    }
  }
  //from role perms
  foreach ($role_permissions as $role_id => $collection_permission) {
    foreach ($collection_permission as $collection_id => $permission) {
      if (($permission_level==NULL || $permission==$permission_level) && in_array($collection_id, $collections)) {
        //populate users per role
        if (!isset($users_via_role[$role_id])) {
          $users_via_role[$role_id]=array();
          $users_query = "SELECT u.uid FROM {users} u, {users_roles} ur, {permission} p
            WHERE        u.uid = ur.uid
            AND          ur.rid = p.rid
            AND          p.rid LIKE '%%%s%'
            ";
          $users_result = db_query($users_query, $role_id);
          while ($row = db_fetch_object($users_result)) {
            $users_via_role[$role_id][]=$row->uid;
          }
        }
        //add the user/collection relationships for each role to the $users list if they are not already on it
        foreach ($users_via_role[$role_id] as $user_id) {
          $user_name=user_load($user_id)->name;
          if (!isset($users[$collection_id])) {
            $users[$collection_id]=array();
          }
          if (!in_array($user_name, $users[$collection_id])) {
            $users[$collection_id][]=$user_name;
          }
        }
      }
    }
  }
  return $users;
}

/**
 * This is a helper function that will search the drupal core database for all users with a specified permission
 * @param string $perm
 *   the permission to search for
 * @return array $users
 *   the list of users with the permission in the form users[user_id]=user_name or false
 */
function user_access_list($perm = NULL) {
  $users = FALSE;
  if (isset($perm)) {
    $users_query = "SELECT u.uid, u.name FROM {users} u, {users_roles} ur, {permission} p
    WHERE        u.uid = ur.uid
    AND          ur.rid = p.rid
    AND          p.perm LIKE '%%%s%'
    ";
    $users_result = db_query($users_query, $perm);
    $users = array();
    while ($u = db_fetch_object($users_result)) {
      $users[$u->uid]=$u->name;
    }
  /*Drupal 6(this may be different in 7) users_roles table does not record authenticated/anonymous/admin user
    *I need to know about authenticated users so, here we manualy check and add them to the list
    */
    $authenticated_users_query = "SELECT rid, perm FROM {permission}
      WHERE       rid = 2
      AND          perm LIKE '%%%s%'
      ";
    $authenticated_users_result = db_query($authenticated_users_query, $perm);

    while ($u = db_fetch_object($authenticated_users_result)) {
      $get_authenticated_users_query = "SELECT uid, name FROM {users}
        WHERE       uid > 0
        ";
      $get_authenticated_users_result = db_query($get_authenticated_users_query);

      while ($u = db_fetch_object($get_authenticated_users_result)) {
        $users[$u->uid]=$u->name;
      }
    }
  }
  return $users;
}

/**
 * This function query's drupal's DB for all the collection permission entries related to the current user
 * Most usefull for building the work portal, and also used for getting user's owned collections
 * @return array $perm_list
 *   list of permissions the current user has with the indicated $perm_list[$role_id][$collection_id]=perms
 */
function islandora_workflow_get_current_users_role_permissions() {
  //do islandora_workflow_get_all_role_permissions with restrictions on the roles to the list of roles the current user has
  $perm_list=array();
  global $user;//use $user->roles to get an array $roles[$role_id]=[$role_name]  $role_id is int

  $roles = array();
  foreach ($user->roles as $rid => $name) {
    $roles[] = $rid;
  }
  
  //build query
  $results = islandora_workflow_get_role_permissions(array('roles' => $roles));

  //build return array
  foreach ($results as $row) {
    $perm_list[$row['role']][$row['collection']] = $row['permission'];
  }
  return $perm_list;
}

/**
* This function query's drupal's DB for all the collection permission entries related to the current user
 * Most usefull for building the work portal, and also used for getting user's owned collections
 * @return array $perm_list
 *   list of permissions the current user has with the indicated $perm_list[$role_id][$collection_id]=perms
 */
function islandora_workflow_get_current_users_user_permissions() {
  //do islandora_workflow_get_all_role_permissions with restrictions on the roles to the list of roles the current user has
  $perm_list=array();
  global $user;

  //build query
  $results = islandora_workflow_get_user_permissions(array('user' => $user->name));

  //build return array
  foreach ($results as $row) {
    $perm_list[$row['user']][$row['collection']] = $row['permission'];
  }
  return $perm_list;
}

/**
 * This function will return a list of all roles that have islandora_workflow permissions
 * @return array $privledged_roles
 *   The list of roles
 */
function islandora_workflow_get_roles_with_module_permissions() {
   $privledged_roles=array();

   //use user_roles for each permission individualy
   $admin_roles=user_roles($membersonly=FALSE, 'islandora_workflow_Administrator');
   $manager_roles=user_roles($membersonly=FALSE, 'islandora_workflow_Manager');
   $editor_roles=user_roles($membersonly=FALSE, 'islandora_workflow_Editor');
   $submitter_roles=user_roles($membersonly=FALSE, 'islandora_workflow_Submitter');

   //modify the returned arrays to include the permission name, and merge into one array [role_pid][role_name][0...4]permission
   foreach ($admin_roles as $role_pid => $role_name) {
     $privledged_roles[$role_pid][$role_name][]='islandora_workflow_Administrator';
   }

   foreach ($manager_roles as $role_pid => $role_name) {
     $privledged_roles[$role_pid][$role_name][]='islandora_workflow_Manager';
   }

   foreach ($editor_roles as $role_pid => $role_name) {
     $privledged_roles[$role_pid][$role_name][]='islandora_workflow_Editor';
   }

   foreach ($submitter_roles as $role_pid => $role_name) {
     $privledged_roles[$role_pid][$role_name][]='islandora_workflow_Submitter';
   }

   return $privledged_roles;
}

/**
 * This function will return a list of all users that have islandora_workflow permissions
 * @return array $privledged_users
 *   The list of users
 */
function islandora_workflow_get_users_with_module_permissions() {
   $privledged_users=array();

   //use user_roles for each permission individualy
   $admin_users=user_access_list('islandora_workflow_Administrator');
   $manager_users=user_access_list('islandora_workflow_Manager');
   $editor_users=user_access_list('islandora_workflow_Editor');
   $submitter_users=user_access_list('islandora_workflow_Submitter');
   //modify the returned arrays to include the permission name, and merge into one array [role_pid][role_name][0...4]permission
   foreach ($admin_users as $user_id => $user_name) {
     $privledged_users[$user_id][$user_name][]='islandora_workflow_Administrator';
   }

   foreach ($manager_users as $user_id => $user_name) {
     $privledged_users[$user_id][$user_name][]='islandora_workflow_Manager';
   }

   foreach ($editor_users as $user_id => $user_name) {
     $privledged_users[$user_id][$user_name][]='islandora_workflow_Editor';
   }

   foreach ($submitter_users as $user_id => $user_name) {
     $privledged_users[$user_id][$user_name][]='islandora_workflow_Submitter';
   }
   return $privledged_users;
}

/**
 * This function is for general workflow permission retrieval on individual
 * non-collection objects.  Please note that this function does not simply report
 * the indicated user's permission level with the parent object.  The function also
 * takes into consideration the workflow state of the object, and only reports those
 * permissions that will allow for altering the state of the object.
 * ie. islandora_workflow_Submitter will not be reported on objects that have the
 * 'published' workflow state.
 * Init is broken up for performance reasons.
 *
 * @param string $object_id
 *   The object that we are looking for permissions on
 * @param object $user_in
 *   The Drupal user whose permissions we are checking or null for the current user
 * @return mixed
 *   Iether the string of the highest available permission to the user on the object or
 *   false if there are no permissions
 */
function islandora_workflow_user_object_permission_check($object_id, $user_in=NULL) {
  module_load_include('inc', 'islandora_workflow', 'islandora_workflow');
  
  //init
  $manager_permissions=islandora_workflow_get_current_users_collections('islandora_workflow_Manager');
  $parent=islandora_workflow_get_object_parent($object_id);

  //if the user has manager level access return true
  if (isset($manager_permissions[$parent])) {
    return 'islandora_workflow_Manager';
  }

  //more init
  $editor_permissions=islandora_workflow_get_current_users_collections('islandora_workflow_Editor');
  $workflow_state=islandora_workflow_get_object_workflow_state($object_id);

  //editor level
  if (isset($editor_permissions[$parent]) && ($workflow_state=='submitted' || $workflow_state=='created')) {
    return 'islandora_workflow_Editor';
  }

  //even more init
  $submitter_permissions=islandora_workflow_get_current_users_collections('islandora_workflow_Submitter');
  $creator=islandora_workflow_get_object_creator($object_id);
  $user_name=NULL;
  //get user name
  if ($user_in==NULL) {
    global $user;
    $user_name=$user->name;
  }
  else {
    $user_name=$user_in->name;
  }

  //submitter level
  if (isset($submitter_permissions[$parent]) && ($workflow_state=='created' || $workflow_state=='rejected' ) && $user_name==$creator) {
    return 'islandora_workflow_Submitter';
  }

  //permission denied
  return FALSE;
}