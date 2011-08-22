<TABLE>
  <THEAD>
    <TR>
    		<TH>
    		</TH>
      <TH>
      Title
      </TH>
      <TH>
      Collection
      </TH>
      <TH>
      Status
      </TH>
      <TH>
      Notes
      </TH>
      <TH>
      Assignee
      </TH>
      <!-- <TH>
      Actions
      </TH>-->
    </TR>
	</THEAD>
	
	<TFOOT>
	</TFOOT>
	
	<TBODY>
	<?php 
	foreach($collections as $collection_id => $collection_name) {
  //$collection_members=get_related_items_as_array($collection_id, 'isMemberOf', 10000 , 0, FALSE);
    $collection_members=islandora_workflow_get_all_members_of_collection($collection_id);
    foreach($collection_members as $member) {
      if (isset($list[$collection_id][$member])) { //The isset is here so that only populated items are displayed on the workflow tab
        print('<TR>');
        print('<TD>'. $list[$collection_id][$member]['Selecter'] .'</TD><TD>'.$list[$collection_id][$member]['object'].
          '</TD><TD>'.$collection_name.'</TD><TD>' . $list[$collection_id][$member]['state'] .
           '</TD><TD>' . $list[$collection_id][$member]['note_subject'] . '</TD><TD>'.$list[$collection_id][$member]['Assign'].'</TD>');
        print('</TR>');
      }
    }
  }
?>
	</TBODY>
	
</TABLE>
<?php 
$pager=theme('pager', NULL, NULL, $pager_index);
print($pager);
?>