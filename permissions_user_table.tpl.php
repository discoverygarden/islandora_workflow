<TABLE>

  <THEAD>
    <TR>
      <TH>
      		User
      </TH>
      <!-- Display a header for each collection -->
      <?php 
        foreach($collections as $collection_id => $collection_name) {
          print('<TH>' . $collection_id .'</BR>' . $collection_name .'</TH>');
        }
      ?>
    </TR>
	</THEAD>
	
	<TFOOT>
	</TFOOT>
	
	<TBODY>
	<!-- Display Table cells for each collection/role intersection -->
		<?php 
		  foreach($user as $user_id => $user_name) {
		    print('<TR><TD>'.key($user_name).'</TD>');
		    foreach($collections as $collection_id =>$collection_name) {
		      print('<TD>'.$list[$user_id][$collection_id].'</TD>');
		    }
		    print('</TR>');
		  }
		  ?>
	</TBODY>
	
</TABLE>