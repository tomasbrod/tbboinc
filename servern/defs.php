<?php

$struct = <<<EOT
authenticator   varchar 32
hostid id
platform varchar 256
alt_platforms vector varchar 256 tag1=alt_platform tag2=name
EOT;

function to_our_array($text)
{
	$result = array();
	foreach( explode("\n",$text) as $line) {
		$rline=array();
		foreach( explode(" ",$line) as $word) {
			if($word=="") continue;
			$kv=explode("=",$word);
			if(sizeof($kv)==2) {
				$rline[$kv[0]]=$kv[1];
			} else {
				$rline[]=$word;
			}
		}
		$result[]=$rline;
	}
	return $result;
}

function generateFieldType($field,$i) {
	if($i>0) $field=array_slice($field, $i, 99999, false);
	if($field[0]=="varchar") {
		echo "char[{$field[1]}]";
	} elseif($field[0]=="id") {
		echo "long";
	} elseif($field[0]=="vector") {
		echo "std::vector< ";
		generateFieldType($field,1);
		echo " >";
	} else {
		echo "//unk. $field[0]";
	}
}

function generateStructBody($arr) {
	foreach($arr as $field) {
		generateFieldType($field,1);
		echo " $field[0];\n";
	}
}

function generateTable($name,$table) {
	echo "const char* {$name}[] = {\n";
	foreach($table as $row) {
		echo "\"$row\",\n";
	}
	echo "};\n";
}

function getSortedTable($arr) {
	$res=array();
	foreach($arr as $field) {
		$res[]=$field[0];
	}
	sort($res);
	return $res;
}

function generateDeserFunction($arr)
{
	
}

$arr=to_our_array($struct);
generateStructBody($arr);
$sorted = getSortedTable($arr);
generateTable("sampleText",$sorted);
