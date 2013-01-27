<?php

/**
* bk_chart_form_wrapper
*/
function bk_chart_form_wrapper($id_key=false, $op='edit', $parent_group=false){
	
	switch($op){
		case 'delete':
			$output=drupal_get_form('bk_chart_form_delete', $id_key);
			break;
		default:
			$output=drupal_get_form('bk_chart_form_edit', $id_key, $parent_group);
			break;
	}
	return $output;
}


/*
* Edit chart record form
*/
function bk_chart_form_edit($form, &$form_state, $id=false, $parent_group=false){
	
	if($id){
		$rec=db_select('bk_chart', 'c')
			->fields('c')
			->condition('gid', $id)
			->execute()
			->fetchObject();
	}
	if(!@$rec) $rec=new stdClass();
	
	$form['gid'] = array(
	  '#type' => 'textfield',
	  '#title' => t('Id'),
	  '#default_value' => @$rec->gid,
	  '#disabled' => @$rec->gid?TRUE:FALSE,
	  '#size' => 6,
	  '#maxlength' =>6,
	  '#required' => TRUE,
	);
	
	if($parent_group){
		$form['gid']['#default_value']=$parent_group;
	}
	
	$form['name'] = array(
	  '#type' => 'textfield',
	  '#title' => t('Name'),
	  '#default_value' => @$rec->name,
	  '#size' => 60,
	  '#maxlength' => 128,
	  '#required' => TRUE,
	);
	
	$form['actions']['submit']=array(
		'#type'=>'submit',
		'#value'=>t('Save'),
		);
		
	if(@$rec->gid){
		$form['actions']['delete']=array(
		'#type'=>'submit',
		'#value'=>t('Delete'),
		);
	}
	$form['actions']['cancel']=array(
		'#type'=>'submit',
		'#value'=>t('Cancel'),
		);
	
	return $form;
}
/**
* Function bk_chart_form_edit_validate()
*/
function bk_chart_form_edit_validate($form, &$form_state){
	
	$v=$form_state['values'];
	
	if($v['op']==t('Save')){
		
		//if account ID is longer than 1 digit - it shall have parent
		if(strlen($v['gid']>1)){
			
			//split appart the ID in separate characters of digits
			$gid_parts=str_split($v['gid']);
			
			$bufgid='';
			
			//build back every parent group ID
			for($i=0; $i<count($gid_parts)-1; $i++){
				$bufgid=$gid_parts[$i];
				
				//if no parent found - complain
				if(!_bk_chart_record_exists($bufgid))
					form_set_error('gid', t('Provided ID has no parents in database. Before registering group under id 123 need to register first higher group under id 1 and subgroup id 12 and only after that 123'));
			}
		}
		
		$gid=$v['gid'];
		
		
		//if it is a new record
		if(!$form_state['build_info']['args'][0]){
			
			//check if record already exists in db
			if(_bk_chart_exists($gid)){
				form_set_error('gid', t('Account group under this ID already exists'));
			}
			
			//check if parent has accounts defined under it
			if(strlen($gid)>1){
				//read parent group id
				$pgid=substr($gid, 0, -1);
				if(_bk_chart_has_accounts($pgid)){
					form_set_error('gid', t('Cannot create this subgroup caouse the parent has accounts registered'));
				}
			}
		}
	}
}
/**
* Function bk_chart_form_edit_submit()
*/
function bk_chart_form_edit_submit($form, &$form_state){
	require_once(drupal_get_path('module', 'bk').'/bk_entry.inc');
	$v=$form_state['values'];
	
	if($v['op']==t('Cancel')){
		$form_state['redirect']='bk/chart';
		return;
	}
	
	if($v['op']==t('Delete')){
		$form_state['redirect']='bk/chart/'.$form_state['build_info']['args'][0].'/delete';
		return;
	}
	
	$id=$form_state['build_info']['args'][0];
	
	if($id){
		$v['__id']='gid';
		$v['__value']=$id;
		bk_entry_update('bk_chart', $v);
		drupal_set_message(t('Entry updated'));
	} else {
		bk_entry_insert('bk_chart', $v);
		drupal_set_message(t('Entry saved'));
	}
	$form_state['redirect']='bk/chart';
}

/**
* Function bk_chart_form_delete()
*/
function bk_chart_form_delete($form, &$form_state, $gid){

	if(_bk_chart_has_children($gid)){
		drupal_set_message(t('This account group has children and shall not be deleted'), 'error');
		drupal_goto('bk/chart');
		return;
	} elseif(_bk_chart_has_accounts($gid)){
		drupal_set_message(t('This account group has accounts and shall not be deleted'), 'error');
		drupal_goto('bk/chart');
		return;
	}
		
	$entry=db_select('bk_chart', 'c')
		->fields('c')
		->condition('gid', $gid)
		->execute()
		->fetchObject();
	
	if(!$entry){
		drupal_goto('bk/chart');
		return;
	}

	$form['gid']=array(
		'#type'=>'hidden',
		'#value'=>$entry->gid,
	);

	$form = confirm_form($form,
		t('Are you sure you want to delete "@name"?', 
		array('@name' => $entry->name)),
		'bk/chart',
		'<p>' . t('Deleting this cannot be undone.') . '</p>',
		t('Delete'),
		t('Cancel'),
		'confirm'
	);
	
  return $form;
}
/**
* Function bk_chart_form_delete_submit()
*/
function bk_chart_form_delete_submit($form, &$form_state){
	require_once(drupal_get_path('module','bk').'/bk_entry.inc');
	$v=$form_state['values'];
	if($v['gid']){
		bk_entry_delete('bk_chart', 'gid', $v['gid']);
		drupal_set_message(t('Entry deleted'));
		
	}
	$form_state['redirect']='bk/chart';
}