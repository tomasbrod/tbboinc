<?php

$struct = <<<EOT
authenticator   varchar 32
hostid id
platform varchar 256
alt_platform1 vector varchar 256 _innertag=name
alt_platform2 vector varchar 256 innertag=name
EOT;

/* types:
 * id - database ID - 'long' s32
 * int - 'int' s32
 * long - same
 * ulong - 'unsigned long' u32
 * uquad - u64
 * bool  - 'bool' u8, or bit
 * element - ...
 * double - 'double' f64
 * float  - 'float'  f32
 * varchar n - 'char[n]' prefix-char
 * string n - 'std::string' prefix-char
 * ignore - no field, skip xml
 * transient (type) - define field, no xml
 * vector (type) vmax=n vmin=n innertag=tag - vector field
*/
/* cardinality:
 * default=(value) - makes the element optional
 * opt=id - otional (0..1), declare variable id and set it
 * lastis=id - optional (0..n), like opt, but allow duplicates and last wins
*/

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
  if($field[0]=="transient") {
    generateFieldType($field,1);
  } elseif($field[0]=="varchar") {
    echo "immstring<$field[1]>";
  } elseif($field[0]=="id") {
    echo "long";
  } elseif($field[0]=="vector") {
    echo "std::vector< ";
    generateFieldType($field,1,3);
    echo " >";
  } elseif($field[0]=="int" or $field[0]=="bool" or $field[0]=="long" or $field[0]=="double" or $field[0]=="float") {
    echo $field[0];
  } elseif($field[0]=="ulong") {
    echo "unsigned long";
  } elseif($field[0]=="uquad") {
    echo "unsigned long long";
  } elseif($field[0]=="string" or $field[0]=="element") {
    echo "std::string";
  } else {
    echo "#error $field[0]";
  }
}

function generateStructBody($arr) {
  foreach($arr as $field) {
    if($field[1]=='ignore') continue;
    echo "  ";
    generateFieldType($field,1,1);
    echo " $field[0];\n";
  }
}

function generateTable($name,$table) {
  echo "const char* ".$name."::tag_table [] = {\n";
  foreach($table as $row) {
    echo " \"$row[0]\",\n";
  }
  echo "};\n\n";
}

function getSorted_compare($l,$r) {
  return strcmp($l[0],$r[0]);
}

function getSorted($arr) {
  $res=$arr;
  usort($res,"getSorted_compare");
  return $res;
}

function generateVectorFieldParse($ref,$field)
{
  $tab = "    ";
  if(isset($field['innertag'])):
?>
    while(1) {
    if(xp.get_tag()) throw EParseXML();
    if(xp.parsed_tag[0]=='/') break;
    if(!strcmp(xp.parsed_tag,"<?=$field['innertag']?>")) {
    <?=$ref?>.emplace_back();
<?php generateFieldParse("$ref.back()",array_slice($field,1)); ?>
    } else xp.unknown_tag();
    }<?php
  else:
    echo $tab."$ref.emplace_back();\n";
    generateFieldParse("$ref.back()",array_slice($field,1));
  endif;
}
  

function generateFieldParse($ref,$field)
{
  $tab = "    ";
  if($field[1]=='varchar') {
    if($field[0]=='vector')
      echo $tab."xp.parse_str($ref.data(), $field[2]);\n";
    else
      echo $tab."xp.parse_str($ref, $field[2]);\n";
  }
  else if($field[1]=='id' or $field[1]=='int' or $field[1]=='long') {
    echo $tab."$ref= xp.parse_long();\n";
  }
  else if($field[1]=='vector') {
    generateVectorFieldParse($ref,$field);
  }
  else if($field[1]=='ignore') {
    echo $tab."xp.ignore_tag();\n";
  }
  else if($field[1]=='bool') {
    echo $tab."$ref= xp.parse_bool();\n";
  }
  else if($field[1]=='string') {
    echo $tab."xp.parse_string($ref);\n";
  }
  else if($field[1]=='float' or $field[1]=='double') {
    echo $tab."$ref= xp.parse_double();\n";
  }
  else echo $tab."#error parse $field[0] $field[1] :)\n";
}

function generateDeserFunction($name,$arr)
{
  ?>
void <?=$name?>::parse(XML_PARSER2& xp)
{
 while(1) {
  if(xp.get_tag()) throw EParseXML();
  if(xp.parsed_tag[0]=='/') break;
  switch(get_index(<?=$name?>::tag_table,sizeof(<?=$name?>::tag_table), xp.parsed_tag)) {
<?php foreach($arr as $index=>$field): ?>
   case <?=$index?>: //<?=$field[0]?>/
<?php generateFieldParse($field[0],$field); ?>
    break;
<?php endforeach; ?>
   default:
    xp.unknown_tag();
  }
 }
 //todo: verify non-optional fields
}

<?php
}

function processStruct($struct,$name,$cpp,$hpp)
{
  $sorted=getSorted($struct);
  if($hpp) {
    ob_start();
    echo "struct $name {\n";
    generateStructBody($struct);
    ?>
  static const char* tag_table [];
  void parse(XML_PARSER2& xp);
};

<?php
    fwrite($hpp,ob_get_clean());
  }
  if($cpp) {
    ob_start();
    generateTable($name,$sorted);
    generateDeserFunction($name,$sorted);
    fwrite($cpp,ob_get_clean());
  }
}

function open_output_file(&$fh,$which,$name)
{
  if($name!='-') {
    $fh=fopen($name,'w');
    if(!$fh) {
      print("Cant open output $which file $name\n");
      exit(1);
    }
  } else {
    if($fh) fclose($fh);
    $fh=false;
  }
}

function processInputFile($filename)
{
  $hpp=$cpp=$name=false;
  $fh= fopen($filename,'r');
  if(!$fh) {
    print("File open error\n");
    exit(1);
  }
  $opt['fh']=$fh;
  while(($line = fgets($fh))!==FALSE) {
    $el=array();
    if(!preg_match("/^[[:space:]]*(.+)[[:space:]]+(.+)/",$line,$el)) continue;
    if($el[1][0]=='#') continue;
    if($el[1]=='hpp')
      open_output_file($hpp,'hpp',$el[2]);
    elseif($el[1]=='cpp') 
      open_output_file($cpp,'cpp',$el[2]);
    elseif($el[1]=='echocpp') {
      if($cpp) fwrite($cpp, $el[2]."\n");
    }
    elseif($el[1]=='echohpp') {
      if($hpp) fwrite($hpp, $el[2]."\n");
    }
    elseif($el[1]=='struct') {
      $name = $el[2];
      $struct=array();
      while(1)
      {
        $line = fgets($fh);
        if(false===$line) {
          print("Unexpected EOF in struct $name\n");
          exit(1);
        }
        $line=trim($line);
        if($line=='' or $line[0]=='#') continue;
        if($line[0]=='%') break;
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
        $struct[]=$rline;
      }
      processStruct($struct,$name,$cpp,$hpp);
    } else {
      echo "Unrecognized line $line\n";
    }
  }
  if($cpp) fclose($cpp);
  if($hpp) fclose($hpp);
}

if(0) {
  $arr=to_our_array($struct);
  $sorted=getSorted($arr);
  generateStructBody($arr);
  generateTable("SampleText",$sorted);
  generateDeserFunction("SampleText",$sorted);
}

if(!empty($argv[1])) {
  processInputFile($argv[1]);
}
