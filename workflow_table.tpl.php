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
      Actions
      </TH>
    </TR>
	</THEAD>
	
	<TFOOT>
	</TFOOT>
	
	<TBODY>
	<?php 
	foreach($collections as $collection_id => $collection_name) {
  $collection_members=get_related_items_as_array($collection_id, 'isMemberOf', 10000 , 0, FALSE);
    foreach($collection_members as $member) {
      if (isset($list[$collection_id][$member])) { //The isset is here so that only populated items are displayed on the workflow tab
        print('<TR>');
        print('<TD>'. $list[$collection_id][$member]['Selecter'] .'</TD><TD>'.$member.'</TD><TD>'.$collection_name.'</TD><TD></TD><TD></TD><TD>');
        print($list[$collection_id][$member]['Edit']);
        print($list[$collection_id][$member]['Manage']);
        print('</TD></TR>');
      }
    }
  }
?>
	</TBODY>
	
</TABLE>