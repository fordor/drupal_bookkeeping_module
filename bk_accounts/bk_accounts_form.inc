<?php

/*
* Function bk_accounts_form_wrapper()
*/
function bk_accounts_form_wrapper ($id, $mode='edit'){
	switch($mode){
		case 'creditrules':
		case 'debitrules':
			return drupal_get_form('bk_accounts_form_transation_rules', $mode);
			break;
		case 'delete':
			return drupal_get_form('bk_accounts_form_delete', $id);
			break;
		default:
			return drupal_get_form('bk_accounts_form_edit', $id, $mode);
			break;
	}
}

/*
* Function bk_accounts_form_edit()
*/
function bk_accounts_form_edit ($form, &$form_state, $id, $mode='open'){
	
	$obj=bk_accounts_get_by_id($id);
	$v=@$form_state['values'];
	
	$rec=NULL;
	
	if($v['op']==t('Cancel')){
		drupal_goto('bk/accounts/'._bk_accounts_get_parent_id($id).'/list');
		return;
	}
		
	drupal_set_breadcrumb(_bk_accounts_breadcrumb($id));
	
	if($mode=='open'){
		
		if($v['op']==t('Cancel')){
			drupal_goto('bk/accounts/'.$id.'/list');
			return;
		}
		
		$form['info'] = array(
			'#type' => 'item',
			'#markup' => '<h2>'.t('Create account under ').$obj->name.' ('.$id.')</h2>',
		);
		
		$form['parent'] = array(
			'#type' => 'hidden', 
			'#value' => $id,
		);
		
		$form['aid'] = array(
			'#type' => 'textfield',
			'#title' => t('Id').': '.$id.'.',
			'#default_value' => @$rec->aid,
			'#size' => 4,
			'#maxlength' => 4,
			'#required' => TRUE,
			'#description' => t('Please take in account the number of digits you specify. For example assuming that you may have tenths of accounts in this compartment please ensure double digits like 01, 02... For hundreds reserve 3 digits 001, 002...'),
			);	
	
	//if edit mode
	}elseif($mode=='edit'){
		$rec=db_select('bk_accounts', 'a')->fields('a')->condition('aid', $id)->execute()->fetchObject();
	}
	
	$form['name'] = array(
	  '#type' => 'textfield',
	  '#title' => t('Account Name'),
	  '#default_value' => @$rec->name,
	  '#size' => 60,
	  '#maxlength' => 128,
	  '#required' => TRUE,
	);
	
		//currency ID
	$qr=db_query("SELECT * FROM {bk_currency}");
	$options=array();
	if($qr)
	while ($c = $qr->fetchObject()) {
		$options[$c->cid]=$acc->currency;
	}
	$options=array_merge(array(
			"USD"=>t("US Dollar"),
			"EUR"=>t("Euro"),
			"RUB"=>t("Russian Rubel"),
		), $options);
	
	//currency 
	$form['cid'] = array(
		'#type' => 'select',
		'#title' => t("Currency"),
		'#options' => $options,
		'#default_value' => isset($rec->cid)?$rec->cid:'',
		'#description' => t('Specify the currency the account amounts will be counted in'),
		'#required' => TRUE,
	);
	
	$form['mu'] = array(
	  '#type' => 'textfield',
	  '#title' => t('Measurement Unit'),
	  '#default_value' => @$rec->mu,
	  '#size' => 12,
	  '#maxlength' => 12,
	  '#required' => FALSE,
		'#description' => t('Measurement units (kg, l, pcs, hours) for physical qantities affiliated with transactions on this account'),
	);
	
	$form['closed']=array(
		'#type'=>'checkbox',
		'#title'=>t('Closed'),
		//'#return_value'=>1,
		'#default_value'=>@$rec->closed,
		);

	$options=array('0'=>t('Credit'), '1'=>t('Debit'));
	$value= $id?variable_get('default_transaction_type_'.str_replace('.', '_', $id)):0;
	if(!$value) $value=0;
	$form['default_transaction_type']=array(
		'#type' => 'radios',
		'#title' => t('Default transaction type'),
		'#options' => $options,
		'#default_value' =>$value,
	);
	
	$options = array(0 => t('Current Month'), 1 => t('Last 2 Months'), 2=>t('All'));
	$value= $id?variable_get('default_time_interval_transaction_'.str_replace('.', '_', $id)):0;
	if(!$value) $value=0;
	$form['default_time_interval_transaction']=array(
		'#type' => 'radios',
		'#title' => t('Default time interval for transactions display'),
		'#options' => $options,
		'#default_value' =>$value,
	);
	
	//fire hooks to allow modifying the form
	foreach (module_implements('bk_accounts_form_edit') as $module) {
		$function = $module . '_bk_accounts_form_edit';
		$function($form, $form_state, $id, $mode);
	}
	
	$form['action']['submit']=array(
		'#type'=>'submit',
		'#value'=>t('Save'),
		);
	
	$form['action']['cancel']=array(
		'#type'=>'submit',
		'#value'=>t('Cancel'),
		);
	
	return $form;
}

/*
* Function bk_accounts_form_edit_validate()
*/
function bk_accounts_form_edit_validate ($form, &$form_state){
	$v=$form_state['values'];
	
	if($v['op']==t('Cancel')||$v['op']==t('Delete')) return;

	if(@$v['parent']&&@$v['aid']){
		$aid=$v['parent'].'.'.$v['aid'];
		$accObj=db_select('bk_accounts', 'a')->fields('a')
			->condition('aid', $aid)->execute()->fetchObject();
			
		if($accObj&&$accObj->aid){
			form_set_error('aid', t('Account ID @aid already exists', array(':aid'=>$accObj->aid)));
		}
	}
	
	//fire hooks to allow modifying the form
	foreach (module_implements('bk_accounts_form_edit_validate') as $module) {
		$function = $module . '_bk_accounts_form_edit_validate';
		$function($form, $form_state);
	}
}
/*
* Function bk_accounts_form_edit_submit()
*/
function bk_accounts_form_edit_submit($form, &$form_state){
	$v=$form_state['values'];
	$id=$form_state['build_info']['args'][0];
	if($v['op']==t('Cancel')){
		if(!_bk_account_has_children($id?$id:$v['parent'].'.'.$v['aid'])){
			$form_state['redirect']='bk/transactions/'.$id.'/list';
		} else {
			$form_state['redirect']='bk/accounts/'.$id.'/list';
		}
		return;
	}
	
	require_once(drupal_get_path('module', 'bk').'/bk_entry.inc');

	if(@$v['parent']&&@$v['aid']){
		$v['aid']=$v['parent'].'.'.$v['aid'];
		$id=$v['aid'];
		bk_entry_insert('bk_accounts', $v);
		drupal_set_message(t('Entry saved'));
		//fire hooks to allow modifying the form
		$type='insert';
	} else {
		$v['__id']='aid';
		$v['__value']=$id;
		
		bk_entry_update('bk_accounts', $v);
		drupal_set_message(t('Entry updated'));
		$type='update';
	}
	
	$key=str_replace('.', '_', $id);
	variable_set('default_time_interval_transaction_'.$key, $v['default_time_interval_transaction']);
	variable_set('default_transaction_type_'.$key, $v['default_transaction_type']);
	
	//fire hooks to allow modifying the form
	foreach (module_implements('bk_accounts_form_edit_submit') as $module) {
		$function = $module . '_bk_accounts_form_edit_submit';
		$function($form, $form_state, $id, $type);
	}
	
}

/*
* Function bk_accounts_form_delete()
*/
function bk_accounts_form_delete ($form, &$form_state, $aid){	
	$parentId=_bk_accounts_get_parent_id($aid);
	
	if(_bk_account_has_children($aid)){
		drupal_set_message(t('This account has children and shall not be deleted'), 'error');
		drupal_goto('bk/accounts/'.$parentId.'/list');
		return;
	} elseif(_bk_account_has_transactions($aid)){
		drupal_set_message(t('This account has transactions and shall not be deleted'), 'error');
		drupal_goto('bk/accounts/'.$parentId.'/list');
		return;
	}
		
	$entry=db_select('bk_accounts', 'a')
		->fields('a')
		->condition('aid', $aid)
		->execute()
		->fetchObject();
	
	if(!$entry){
		drupal_goto('bk/accounts/'.$parentId.'/list');
		return;
	}

	$form['aid']=array(
		'#type'=>'hidden',
		'#value'=>$entry->aid,
	);

	$form = confirm_form($form,
		t('Are you sure you want to delete "@name"?', 
		array('@name' => $entry->name)),
		'bk/accounts/'.$parentId.'/list',
		'<p>' . t('Deleting this cannot be undone.') . '</p>',
		t('Delete'),
		t('Cancel'),
		'confirm'
	);
	
  return $form;
}

/*
* Function bk_accounts_form_delete_submit()
*/
function bk_accounts_form_delete_submit ($form, &$form_state){
	
	require_once(drupal_get_path('module','bk').'/bk_entry.inc');
	$v=$form_state['values'];
	
	if($v['aid']){
		bk_entry_delete('bk_accounts', 'aid', $v['aid']);
		drupal_set_message(t('Entry deleted'));
		
		//fire hooks to allow modifying the form
		foreach (module_implements('bk_accounts_form_delete_submit') as $module) {
			$function = $module . '_bk_accounts_form_delete_submit';
			$function($form, $form_state, $type);
		}
	}
	
	$parentId=_bk_accounts_get_parent_id($v['aid']);
	$form['redirect']='bk/accounts/'.$parentId.'/list';
}
