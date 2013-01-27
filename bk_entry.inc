<?php

/***************************************************************
 * Bookeeping database entries handling
 ***************************************************************/
 
 /*
 * Funcition bk_entry_clean()
 * Cleans an array of entries to fit schemas fields
 */
 function bk_entry_clean ($schema, $entry, $extended=false)
 {
	
	//load schema information
	$schema=drupal_get_schema($schema);
	
	//extract field keys
	$flds=array_fill_keys(array_keys($schema['fields']), 0);
	//print_r($flds);
	//if first element of the entry is an array
	if(is_array(current($entry))){
	
		//repeat for each element this function
		foreach($entry as $k=>$e)	{
			$entry[$k]=array_intersect_key($e, $flds);
		}
	
	} else {
	
		//limit the entry to schema key elements
		$entry=array_intersect_key($entry, $flds);
	}
	
	if($extended){
		return array($entry, $flds, $schema['primary key']);
	}
	return $entry;
 }
 
 /**
 * Function bk_get_sql_set()
 */
 
 function bk_get_sql_set($data, $primaryK=false){
	$r=new stdClass();
	$r->setkeys=array();
	$r->values=array();
	foreach($data as $k=>$v){
		$r->setkeys[':'.$k]="`$k`=:$k";
		$r->values[':'.$k]=$v;
	}
	$r->set=implode(', ', $r->setkeys);
	
	if($primaryK){
		$r->updatekeys=$r->setkeys;
		$r->updatevalues=$r->values;
		foreach($primaryK as $k){
			unset($r->updatekeys[':'.$k]);
			unset($r->updatevalues[':'.$k]);
		}
		
		$r->update = implode(', ', $r->updatekeys);
	}
	return $r;
 }
 

/**
 * Insert an entry in the database.
 */
 
 function bk_entry_insert($schema, $entry) {
  
	//clean entry data from additional fields
	$entry=bk_entry_clean($schema, $entry);
	
	//check if multiple entries are passed to be saved
	$isArr=is_array(current($entry));
	
	//read field names
	$fields=array_keys($isArr?current($entry):$entry);

	//insert data into db
	$query = db_insert($schema)->fields($fields);
	
	//if entry is an array
	if($isArr) {
		//append each entry element as values to insert
		foreach($entry as $v) $query->values($v);
	}
	
	//if is just one entry - appen it only
	else $query->values($entry);
	
	//execute query
	return $query->execute();
}

/**
 * Update an entry in the database.
 * $entry['__id'] and $entry['__value'] shall be present to initiate update
 */
 
 function bk_entry_update($schema, $entry) {
	
	$return_value = NULL;
	$isOK=TRUE;

	if(is_array(current($entry))){
		foreach($entry as $v) {
			if(!bk_entry_update($v)) $isOK=FALSE;
		}
	//updates will be performed only if old id is present 
	} elseif(!empty($entry['__id'])||!empty($entry['__value']))  {
		$id=$entry['__id'];
		$value=$entry['__value'];
		try {
			$return_value = db_update($schema)
											->fields(bk_entry_clean($schema, $entry))
											->condition($id, $value)
											->execute();
			}
			catch (Exception $e) {
				drupal_set_message(t('Failed to update entry.'), false, 'error');
			}
	} else {
		drupal_set_message('Entry without __id and __value keys detected in update function', false, 'error');
	}
	return $return_value && $isOK;
}

/*
* Function bk_entry_insert_update()
*/
function bk_entry_insert_update ($schema, $entry, $track=false){
	//clean entry data from additional fields
	list($entry, $fields, $primary)=bk_entry_clean($schema, $entry, true);
	
	//if is set to append uid and created time fields
	if($track){
		if(in_array('uid', $fields)){
			global $user; $entry['uid']=$user->uid;
		}
		if(in_array('created', $fields)){
			$entry['created']=time();
		}
	}
	
	$set=bk_get_sql_set($entry, $primary);
	
	$sql="INSERT INTO {$schema} SET ".$set->set." ON DUPLICATE KEY UPDATE ".$set->update;
	
	return db_query($sql, $set->values);
}
/**
 * Delete an entry from the database.
 */
function bk_entry_delete($schema, $key, $ids) {
	
	//if $ids are not array - make them array
	if(!is_array($ids)) $ids=array($ids);
	
	//if strings in $ids - quote them
	foreach($ids as $k=>$v){
		if(!is_numeric($v)) $ids[$k]="'$v'";
	}
	
	//check if anything is in db to delete
	$eids = array();
	
	//delete records from db
	$query = db_delete($schema)->where($key.' IN (' . implode(',', $ids) . ')');
	return $query->execute();
}