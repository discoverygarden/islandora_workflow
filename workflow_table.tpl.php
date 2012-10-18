<?php
/**
 * @file
 *   template file for the workflow table
 */

?>
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
      Object Created
      </TH>
      <TH>
      Last Workflow Movement
      </TH>
      <TH>
      Notes
      </TH>
      <TH>
      Assignee
      </TH>
  	</TR>
  </THEAD>

	<TFOOT>
	</TFOOT>

	<TBODY>
	<?php
      foreach($collections as $collection_id => $collection_names_and_members) :
        foreach($collection_names_and_members as $collection_name => $collection_members) :
          foreach($collection_members as $member => $member_attributes) :
            if (isset($list[$collection_id][$member])) :
              /* The isset is here so that only populated items are
                 displayed on the workflow tab.*/
              print('<TR>');
              print('<TD>' . $list[$collection_id][$member]['Selecter'] . '</TD><TD>' . $list[$collection_id][$member]['object'] .
                '</TD><TD>' . $collection_name . '</TD><TD>' . $list[$collection_id][$member]['state'] .
                '</TD><TD>' . $list[$collection_id][$member]['when_created'] . '</TD><TD>' . $list[$collection_id][$member]['last_workflow_progression'] .
                '</TD><TD>' . $list[$collection_id][$member]['note_subject'] . '</TD><TD>' . $list[$collection_id][$member]['Assign'] . '</TD>');
                print('</TR>');
            endif;
          endforeach;
        endforeach;
      endforeach;
    ?>
	</TBODY>

</TABLE>

