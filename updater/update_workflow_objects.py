'''
This file is meant to give an upgrade path to the new schema.
The changes are:
1. The current note subject will be mirrored in the RELS-INT
2. The timestamp for when the workflow was last modified will be mirrored in the RELS-INT

@author William Panting
@date December 1 2011
@todo: add more logging
@todo: add more error checking

'''

import logging
import ConfigParser
import sys
import os
import urllib2
import urllib
from lxml import etree
from time import strftime
from fcrepo.connection import Connection
from fcrepo.connection import FedoraConnectionException
from fcrepo.client import FedoraClient
from islandoraUtils.metadata import fedora_relationships

def get_workflow_objects(config_dict):
    '''
    This function will get all the objects that are in a workflow state.
    @param config_dict
      The dictionary of script config settings
    @return workflow_object_pids
      The list of all workflow objects
    '''
    workflow_object_pids = list()
    
    query_file_path = os.path.join(os.path.dirname(__file__), 'workflow_object_query.txt')
    query_file_handle = open(query_file_path,'r')
    query_string = query_file_handle.read()
    query_file_handle.close()
    
    request_url = config_dict['fedora_url'] + '/risearch'
    request_data= {
                  'type':'tuples',
                  'flush':'TRUE',
                  'format':'Sparql',
                  'offset':'0',
                  'lang':'sparql',
                  'query':query_string
                  }
    
    encoded_request_data = urllib.urlencode(request_data)
    query_response = urllib2.urlopen(request_url, encoded_request_data)
    query_response_string = query_response.read()
    
    query_response_etree = etree.fromstring(query_response_string)
    uri_list = query_response_etree.xpath('//@uri')
    
    #turn the uri_list into a pid list
    for uri in uri_list:
        workflow_object_pids.append(uri[12:])
        
    return workflow_object_pids

def get_workflow_info(fcrepo_object):
    '''
    This function will return the parts of the workflow datastream that are 
    to be pushed to the RELS-INT
    @param fcrepo_obj
      The fcrepo object to get the data about.
    @param connection
      The fcrepo connection to use when getting the workflow datastream.
    @return dict object_workflow_attributes 
      This is the info taken from the workflow datastream
      {'timestamp':'', 'subject':''}
    '''
    object_workflow_attributes = dict()
    
    timestamp = ''
    subject = ''
    
    workflow_datastream = fcrepo_object['islandora_workflow']
    workflow_string = workflow_datastream.getContent().read()
    workflow_etree = etree.fromstring(workflow_string)
    timestamp_list = workflow_etree.xpath('//timestamp')
    subject_list = workflow_etree.xpath('//subject')
    
    object_workflow_attributes = {'timestamp':timestamp_list[0].text, 'subject':subject_list[0].text}
    
    return object_workflow_attributes

def update_object(fcrepo_object, object_workflow_attributes):
    '''
    This function will actualy update the fedora object with the new RELS-INT
    @param fcrepo_object
    '''
    #info:islandora/islandora-system:def/islandora_workflow#
    
    islandora_workflow_namespace = fedora_relationships.rels_namespace('islandora_workflow','info:islandora/islandora-system:def/islandora_workflow#')
    object_RELS_INT = fedora_relationships.rels_int(fcrepo_object, islandora_workflow_namespace, 'islandora_workflow')
    object_RELS_INT.addRelationship('islandora_workflow','has_note_subject', [object_workflow_attributes['subject'], 'literal'])
    object_RELS_INT.addRelationship('islandora_workflow','has_workflow_timestamp', [object_workflow_attributes['timestamp'], 'literal'])
    object_RELS_INT.update()

def update_objects(connection, config_dict):
    '''
    This function will actualy do the work to update the fedora workflow objects
    @param connection
      The connection(fcrepo) to fedora.
    @param config_digt
      The data extracted from the config parser.
    '''
    logging.info('Getting objects to update. \n')
    workflow_object_pids = get_workflow_objects(config_dict)
    
    logging.info('Begining updating of fedora objects'+'\n')
    
    fedora = FedoraClient(connection)
    
    for pid in workflow_object_pids:
        fcrepo_object = fedora.getObject(pid)
        
        object_workflow_attributes = get_workflow_info(fcrepo_object)
        logging.info('Info for object ' + pid + 'retrieved. \n')
        update_object(fcrepo_object, object_workflow_attributes)
        logging.info('Object '+ pid + ' updated. \n')
        
    logging.info('Objects updated.\n')
    return True
    
if __name__ == '__main__':
    '''
    If it's the main script this module will get the config file from the arguments,
    set up the fedora conneciton and logging and call the updater with the info it needs.
    '''
    #read args
    if len(sys.argv) == 2:
        config_file_path = sys.argv[1]
    else:
        print('Please verify configuration file is present and entered correctly.')
        sys.exit(-1)
    
    #get config
    config = ConfigParser.ConfigParser()
    config.read(config_file_path)
    config_dict = dict()
    config_dict['fedora_url'] = config.get('Fedora','url')
    config_dict['fedora_user_name'] = config.get('Fedora', 'username')
    config_dict['fedora_password'] = config.get('Fedora', 'password')
    config_dict['log_directory'] = config.get('Logging', 'directory')
    config_dict['log_level'] = config.get('Logging', 'level')
    
    #get fedora connection
    connection = Connection(config_dict['fedora_url'],
                    username = config_dict['fedora_user_name'],
                     password = config_dict['fedora_password'])
    
    #configure logging
    if not os.path.isdir(config_dict['log_directory']):
        os.mkdir(config_dict['log_directory'])
    logFile = os.path.join(config_dict['log_directory'],'update_workflow_objects' + strftime('_%y_%m_%d_%H_%M') + '.log')
    
    if (config_dict['log_level'] == 'DEBUG'):
        log_level_code = logging.DEBUG
    if (config_dict['log_level'] == 'INFO'):
        log_level_code = logging.INFO
    if (config_dict['log_level'] == 'WARNING'):
        log_level_code = logging.WARNING
    if (config_dict['log_level'] == 'ERROR'):
        log_level_code = logging.ERROR
    if (config_dict['log_level'] == 'CRITICAL'):
        log_level_code = logging.CRITICAL
    else:
        log_level_code = logging.ERROR
        logging.basicConfig(filename = logFile, level = log_level_code)
        logging.warning('An invalid logging level was declared in the config file.\n')
        
    logging.basicConfig(filename = logFile, level = log_level_code)
 
    #run upater
    update_objects(connection, config_dict)