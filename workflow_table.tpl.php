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
    }
?>
	</TBODY>
	
</TABLE>