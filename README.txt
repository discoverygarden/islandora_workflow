This is a drupal module to implement a generic workflow around Fedora objects.

It uses an inline xml datastream to record the state of the workflow and notes associated with the workflow in workflow managed objects.
The module currently assumes that the 'islandora:collectionCModel' object acts as a parent for collection content models and not directly as a content model.  This can be changed via modifying 'collection_query.txt'.

Machine level workflow will be handled via fedora microservices.
Human level workflow will be handled through this drupal module.

Acceptable state values: created, in_processing(for machine workflow), submitted, approved, published, rejected

Workflow can be enforced through the use of XACML. There is a settings toggle for this.