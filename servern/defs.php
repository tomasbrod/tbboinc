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
	echo "const char* ".$name."::tag_table [] = {\n";
	foreach($table as $row) {
		echo " \"$row[0]\",\n";
	}
	echo "};\n";
}

function getSorted_compare($l,$r) {
	return strcmp($l[0],$r[0]);
}

function getSorted($arr) {
	$res=$arr;
	usort($res,"getSorted_compare");
	return $res;
}

function generateDeserFunction($name,$arr)
{
	echo "void parse_struct(XML_PARSER2& xp, $name &st) {\n";
	echo " while(xp.get_tag()) {\n";
	echo "  switch(get_index($name::tag_table,sizeof($name::tag_table), xp.tag)) {\n";
	foreach($arr as $index=>$field) {
		echo "   case $index:\n";
		echo "    xp.parse_str(st.$field[0], sizeof(st.$field[0]));\n";
		echo "    break;\n";
	}
	echo "   default: xp.skip();\n";
	echo "  }\n }\n}\n\n";
}

$arr=to_our_array($struct);
generateStructBody($arr);
$sorted = getSorted($arr);
generateTable("SampleText",$sorted);
generateDeserFunction("SampleText",$sorted);
