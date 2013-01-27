<?php

require_once drupal_get_path('module', 'bk').'/bk_entry.inc';

/***************************************************************
 * Import Account Groups Form	
 ***************************************************************/
 
/*
* Form bk_chart_import_form_()
*/
function bk_chart_import_form($form, &$form_state){
	
	//source directory for txt files that contain ready accounting charts
	$src_dir=drupal_get_path('module', 'bk_chart').'/charts';
	
	//load chart files
	$files=glob($src_dir.'/*.txt');
	
	if(!$files){
		$form['info'] = array(
		  '#type' => 'item',
		  '#markup' => t('No chart files available in: ').$src_dir,
		);
		
		return $form;
	}
		
	$options=array();
	//get bare file names
	foreach($files as $f){
		$k=basename($f, ".txt");
		$options[$k]=$k;
	}
	
	//get current source file to load
	$v=@$form_state['values'];	
	$source_file=@$v['source']?$v['source']:current($options);
	
	//group ID selector
	$form['source'] = array(
		'#type' => 'select',
		'#title' => t('Source File'),
		'#default_value' => $source_file,
		'#options' => $options,
		'#ajax' => array(
			'callback' => 'bk_chart_import_ids_callback',
			'wrapper' => 'parrent-accounts-div',
			'effect' => 'slide',
			'progress' => array('type' => 'none'),
		),
		'#required' => TRUE,
	);
	
	//load current file	
	$lines=file($src_dir.'/'.$source_file.'.txt');
	
	$rows=array();
	
	foreach($lines as $l){
		list($code, $nm)=explode("\t", trim($l));
	
		$rows[$code]=array(
			'gid'=>array(
				'data'=>$code,
				'class'=>array(),
			),
			'groupname'=>array(
				'data'=>$nm,
				'class'=>array('bk-a'.strlen($code)),
				),
		);
	}
	
	
	//mark rows that esists in DB
	$ids=array_keys($rows);
	if($ids){
		$ids=db_select('bk_chart', 'c')
			->fields('c', array('gid'))
			->where('gid IN ('.implode(', ',$ids).')')
			->execute()->fetchAllAssoc('gid', PDO::FETCH_OBJ);
		if($ids)
		foreach($ids as $id) $rows[$id->gid]['gid']['class']+=array('bk-selected');
	}

	$form['ids']=array(
		'#type'=>'tableselect',
		'#header'=>array(
			'gid'=>t('Id'), 
			'groupname'=>t('Name'),
		),
		'#options'=>$rows,
		'#required' => TRUE,
		'#prefix' => '<div id="parrent-accounts-div">',
		'#suffix' => '</div>',
	);
	

	$form['submit']=array(
	'#type'=>'submit',
	'#value'=>t('Import'),
	);
	
	return $form;
}

/*
* Funcition bk_chart_import_ids_callback()
*/
function bk_chart_import_ids_callback ($form, &$form_state)
{
	return $form_state['ids'];
}

/*
* Submit for Form bk_chart_import_form_
*/
function bk_chart_import_form_submit($form, &$form_state){

	//source directory for txt files that contain ready accounting charts
	$src_dir=drupal_get_path('module', 'bk_chart').'/charts';
	
	//get current source file to load
	$v=@$form_state['values'];	
	$source_file=@$v['source'];	
	$source_file=drupal_realpath($src_dir.'/'.$source_file.'.txt');
	
	if($source_file){
		
		//load current file	
		$lines=file($source_file);
		$sources=$toinsert=array();
		foreach($lines as $l){
			list($code, $nm)=explode("\t", trim($l));
			$sources[$code]=array('gid'=>$code, 'name'=>$nm);
		}
		
		//load existing database rows
		$dbrows=db_select('bk_chart', 'c')->fields('c')->execute()->fetchAllKeyed(0,1);
		
		
		foreach($v['ids'] as $v){
			if($v!=0){
				$keys = str_split($v);
				$code='';
				foreach($keys as $k){
					$code.=$k;
					if(!isset($dbrows[$code])){
						$toinsert[$code]=array('gid'=>$code, 'name'=>$sources[$code]['name']);
					}
				}
			}
		}
		
		bk_entry_insert('bk_chart', $toinsert);	
	
		$msg=array();
		foreach($toinsert as $v) $msg[]=$v['gid'].': '.$v['name'];
		drupal_set_message(t('Imported').' '.implode(', ', $msg));
		
	} else {
		drupal_set_message(t('No source file'), 'error');
	}
	//$form_state['redirect']='admin/bk_chart_import/list';
}