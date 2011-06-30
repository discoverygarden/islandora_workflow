This is a drupal module to implement a generic workflow around Fedora objects.

It uses an inline xml datastream to record the state of the workflow and notes associated with the workflow in workflow managed objects.
Acceptable state values: created, in_process(for machine workflow), submitted, approved, published, rejected.

The module currently assumes that the 'islandora:collectionCModel' object acts as a parent for collection content models and not directly as a content model.
This can be changed via modifying 'collection_query.txt'.

Machine level workflow will be handled via fedora microservices.
Human level workflow will be handled through this drupal module.

Workflow can [not implemented yet] be enforced through the use of XACML. There is a settings toggle for this.
This toggle is volatile, it may cause many problems.  We highly recommend only changing it at install time.

What happens when a Drupal admin revokes a permission that someone currently has with a collection? 
Please make sure not to do this: remove the workflow perms first to ensure database sync.

How do we recover if Drupal side modifications occured when Fedora was inactive? Please don't do this, be mindful of fedora's state.

To be able to set user specific roles without restriction you need to set authenticated user permissions to islandora_workflow_Submitter, islandora_workflow_Editor and islandora_workflow_Manager permissions.
Also, anyone who is to edit other users permissions will need the permission 'access administration pages'.
Do not set the islandora_workflow_Administrator permission for the authenticated user role.
If a user has two levels or more of permissions on the same collection the highest level takes precedence.

You may need to delete/modify the 'permit-anything-to-administrator.xml' (Fedora xacml)file.

To start an object in the workflow call this function from islandora_workflow.inc: islandora_workflow_init_workflow($object_id)