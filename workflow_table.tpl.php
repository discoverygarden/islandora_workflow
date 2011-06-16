<TABLE>
  <THEAD>
    <TR>
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
	dsm($list);
	foreach($collections as $collection_id => $collection_name) {
  $collection_members=get_related_items_as_array($collection_id, 'isMemberOf', 10000 , 0, FALSE);
    foreach($collection_members as $member) {
      if (isset($list[$collection_id][$member])) { //The isset is here so that only populated items are displayed on the workflow tab
        print('<TR>');
        print('<TD>'.$member.'</TD><TD>'.$collection_name.'</TD><TD></TD><TD></TD><TD>');
        print($list[$collection_id][$member]);
        print('</TD></TR>');
      }
    }
  }/*
    foreach ($list as $collection_id => $list_element) {
      if(is_array($list_element)) {
        foreach ($list_element as $object_id => $objects) {
            print('<TR>');
            print('<TD>'.$object_id.'</TD><TD>'.$collection_id.'</TD><TD></TD><TD></TD><TD>');
          foreach ($objects as $button_id => $button) {
            print($button); 
          }
          print('</TD></TR>');
        }
      }
    }*/
?>
	</TBODY>
	
</TABLE>