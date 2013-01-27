<?php

/*
* Funcition bk_transactions_form_form_wrapper()
*/
function bk_transactions_form_wrapper ($mode='edit', $tid=false, $aid=false){

	switch ($mode){
		case 'delete':
			return drupal_get_form('bk_transactions_form_delete', $tid, $aid);
			break;
		default:
			return drupal_get_form('bk_transactions_form_edit', $tid, $aid);
			break;
	}
}
/*
* Funcition bk_transactions_form_edit()
*/
function bk_transactions_form_edit ($form, &$form_state, $tid=false, $aid=false){
	
	if($aid)
		drupal_set_breadcrumb(_bk_accounts_breadcrumb($aid));
	
	//if transation ID is defined - load the transaction data from database
	if($tid){
		$transaction=db_select('bk_transactions', 't')
			->fields('t')->condition('tid', $tid)
			->execute()->fetchObject();
		//if($transaction) $aid=$transaction->accfrom;
	//if no transation ID = just create a new object (for fun is ok? :)
	} else {
		$transaction=new stdClass();
	}
	
	//settings key
	$sk=str_replace('.', '_', $aid);
	//print $aid;
	//exit;
	
	//if have account id defined
	if($aid){
		$account=db_select('bk_accounts', 'a')
			->fields('a')->condition('aid', $aid)
			->execute()->fetchObject();
	}
	
	$form['main_data'] = array(
		'#prefix' => '<div class="container-inline">',
	  '#type' => 'container',
		'#suffix' => '</div>',
	);
	
	//ask user to choose credit/debit the account
	$options=array('0'=>t('Credit'), '1'=>t('Debit'));
	
	$form['main_data']['type'] = array(
		//'#prefix' => '<div class="container-inline">',
		'#type' => 'radios',
		'#title' => t('Transaction Type').': ',
		'#default_value' => @$transaction->accto==$aid?'1':variable_get('default_transaction_type_'.$sk, 0),
		'#options' => $options,
		//'#suffix' => '</div>',
	);
	
	// Timestamp of the transation - when it occurs
  $form['tdate'] = array(
    '#title' => t('Date'),
    '#type' => 'textfield',
    '#default_value' => date('Y-m-d H:i', @$transaction->tdate?$transaction->tdate:time()),
    '#required' => TRUE,
    //'#weight' => -5,
		'#description'=>"",
  );
	
	$form['main_data']['locked']=array(
		'#type'=>'checkbox',
		'#title'=>t('Locked'),
		//'#return_value'=>1,
		//'#default_value'=>$rec->locked,
		'#access' => user_access('admin bookkeeping') ,
		'#default_value' => @$transaction->locked,
		);
	
	
	// Use the date_popup module if it's available.
  if (module_exists('date_popup')) {
    $form['tdate']['#type'] = 'date_popup';
    $form['tdate'] += array(
      '#date_format' => 'Y-m-d H:i',
      '#date_type' => DATE_UNIX,
      '#date_timezone' => date_default_timezone(),
			'#date_label_position'=>FALSE,
    );
  }
	
	
	
	//select accounts that can credit this account
	$options=array('0'=>'Openning Ballance');
	
	$sql="SELECT a.aid, a.name FROM {bk_accounts} a
		LEFT JOIN {bk_accounts} j ON j.aid LIKE CONCAT(a.aid, '.%')";
		
	$options=db_query("SELECT DISTINCT 
		a1.aid as 'id', 
		GROUP_CONCAT(a2.name SEPARATOR  '->') as 'name' 
	FROM 
		bk_accounts a1
		LEFT JOIN bk_accounts j ON j.aid LIKE CONCAT(a1.aid, '.%')
		LEFT JOIN (
			SELECT r.`idfrom` as 'idexp' 
			FROM bk_rules r 
			WHERE :aid REGEXP CONCAT('^', REPLACE(r.idto, '.', '\\\.'))		
			) as rl ON a1.aid REGEXP CONCAT('^', REPLACE(rl.idexp, '.', '\\\.')), 
		bk_accounts a2 
	WHERE 
		a1.aid LIKE CONCAT(a2.aid, '%')
		and j.aid is null
		and rl.idexp is not null
	GROUP BY a1.aid
	ORDER BY a1.aid and LENGTH(a1.aid)", array(':aid'=>$aid))->fetchAllKeyed(0, 1);

	//show selector for accounts that can credit - but only visible as credit radio selected
	$form['idfrom'] = array(
    '#type' => 'select',
    '#title' => t('From Account'),
		'#options'=>array('0'=>'Ballance Openning')+$options,
    '#states' => array(
      'visible' => array(
        ':input[name="type"]' => array('value' => '1'),
      ),
    ),
		'#default_value' => @$transaction->accfrom,
		
	);
	 
	//select accounts that can debit this account
	$options=db_query("SELECT DISTINCT 
		a1.aid as 'id', 
		GROUP_CONCAT(a2.name SEPARATOR  '->') as 'name' 
	FROM 
		bk_accounts a1
		LEFT JOIN bk_accounts j ON j.aid LIKE CONCAT(a1.aid, '.%')
		LEFT JOIN (
			SELECT r.`idto` as 'idexp' 
			FROM bk_rules r 
			WHERE :aid REGEXP CONCAT('^', REPLACE(r.idfrom, '.', '\\\.'))		
			) as rl ON a1.aid REGEXP CONCAT('^', REPLACE(rl.idexp, '.', '\\\.')), 
		bk_accounts a2 
	WHERE 
		a1.aid LIKE CONCAT(a2.aid, '%')
		and j.aid is null
		and rl.idexp is not null
	GROUP BY a1.aid
	ORDER BY a1.aid", array(':aid'=>$aid))->fetchAllKeyed(0, 1);

	//cshow debit account selector - but only id debit radio checked
	$form['idto'] = array(
    '#type' => 'select',
    '#title' => t('To Account'),
		'#options'=>array('0'=>'Ballance Openning')+$options,
    '#states' => array(
      'visible' => array(
        ':input[name="type"]' => array('value' => '0'),
      ),
    ),
		'#default_value' => @$transaction->accto,
	);
	
	$form['quantitative_data'] = array(
		'#prefix' => '<div class="container-myinline">',
	  '#type' => 'container',
		'#suffix' => '</div><div class="container-myinline-stop"></div>',
	);
	
	//the amount of money field
	$form['quantitative_data']['amount'] = array(
	  '#type' => 'textfield',
	  '#title' => t('Amount').' ('.@$account->cid.')',
	  '#default_value' => @$transaction->amount,
	  '#size' => 10,
	  '#maxlength' => 15,
	  '#required' => TRUE,
		'#attributes'=>array(
				'class'=>array('bk-nr'),
			),
	);
	
	
	
	//if quantitative measurement unit is available - ask to input quantity too
	if(@$account->mu){
		$form['quantitative_data']['q'] = array(
			'#type' => 'textfield',
			'#title' => t('Quantity').' ('.$account->mu.')',
			'#default_value' => @$transaction->q,
			'#size' => 10,
			'#maxlength' => 15,
			'#required' => TRUE,
			'#attributes'=>array(
				'class'=>array('bk-nr'),
			),
		);
	}

	//ask for document number if any
	$form['quantitative_data']['docnr'] = array(
	  '#type' => 'textfield',
	  '#title' => t('Document Number'),
	  '#default_value' => @$transaction->docnr,
	  '#size' => 15,
	  '#maxlength' => 25,
	  '#required' => FALSE,
	);
	
	//and a little commment
	$form['comment'] = array(
	  '#type' => 'textfield',
	  '#title' => t('Comment'),
	  '#default_value' => @$transaction->comment,
	  '#size' => 60,
	  '#maxlength' => 128,
	  '#required' => TRUE,
	);
	
	
	
	//decide which 2 accounts shall load extenders at the moment
	//(taking in account that ajax may be engaged as well)
	//if no account id
	foreach (module_implements('bk_transactions_form_edit') as $module) {
		$function = $module . '_bk_transactions_form_edit';
		$function($form, $form_state, $tid, $aid, $transaction, $account);
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
* Funcition bk_transactions_form_edit_submit()
*/
function bk_transactions_form_edit_submit($form, &$form_state){
	
	require_once(drupal_get_path('module', 'bk').'/bk_entry.inc');
	
	$tid=$form_state['build_info']['args'][0];
	$aid=$form_state['build_info']['args'][1];
	
	$type=$form_state['values']['type'];
	
	//debit type
	if($type=='1'){
		$form_state['values']['accfrom']=$form_state['values']['idfrom'];
		$form_state['values']['accto']=$aid;
	//credit type
	} else {
		$form_state['values']['accfrom']=$aid;
		$form_state['values']['accto']=$form_state['values']['idto'];
	}
	
	$form_state['values']['tdate']=strtotime($form_state['values']['tdate']);
	
	$v=$form_state['values'];

	//if no transaction ID - new entry
	if(!$tid){
		$tid=bk_entry_insert('bk_transactions', $v);
		if($tid) drupal_set_message(t('Transaction saved'));
		else drupal_set_message(t('Failed to save transaction'), 'error');
		$form_state['values']['tid']=$tid;
	} else {
		$v['__id']='tid';
		$v['__value']=$tid;
		$form_state['values']['tid']=$tid;
		bk_entry_update('bk_transactions', $v);
		drupal_set_message(t('Transaction updated'));
	}
	
	if($tid) {
		//fire hooks to allow modifying the form submit action
		foreach (module_implements('bk_transactions_form_edit_submit') as $module) {
			$function = $module . '_bk_transactions_form_edit_submit';
			$function($form, $form_state);
		}
	}
}