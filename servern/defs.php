<?php

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

class Struct
{
	public $name=NULL;
	public $fields=array();
	public $attrs=array();
	public $tags=array();
	public $content=NULL;
	public $tagname=NULL;
	public $flags=0;
	public $enums=array();
	public $optflags=array();
};

function generateFieldType($struct,$field,$i) {
	$type=$field->type;
	if($type[$i]=="varchar") {
		echo "immstring<".($type[$i+1]+1).">";
		if(isset($field->def)) $field->def= '"'.$field->def.'"';
	} elseif($type[$i]=="id") {
		echo "long";
	} elseif($type[$i]=="vector") {
		echo "std::vector< ";
		generateFieldType($struct,$field,$i+1);
		echo " >";
		$field->def='.clear()';
	} elseif($type[$i]=="int" or $type[$i]=="bool" or $type[$i]=="long" or $type[$i]=="double" or $type[$i]=="float") {
		echo $type[$i];
	} elseif($type[$i]=="ulong") {
		echo "unsigned long";
	} elseif($type[$i]=="uquad") {
		echo "unsigned long long";
	} elseif($type[$i]=="string" or $type[$i]=="element") {
		echo "std::string";
		if(isset($field->def)) $field->def= '"'.$field->def.'"';
	} elseif($type[$i]=="boolp") {
		echo "bool";
		$field->def=0;
	} elseif($type[$i]=="struct") {
		echo $type[$i+1];
	} elseif($type[$i]=="structn") {
		echo $struct->name."_".$field->name;
		//$type[$i]='struct';
		//$type[$i+1]=$struct->name."_".$field->name;
	} elseif($type[$i]=="enum") {
		echo "short";
		$field->enum = explode(',',$type[$i+1]);
		$struct->enums += array_flip($field->enum);
		if(isset($field->def)) $field->def= 'v_'.$field->def;
	} elseif($type[$i]=="halt") {
		echo "void";
		$field->serialize=false;
		//$field->optx=$struct->flags;
		//$struct->flags++;
	} elseif($type[$i]=="named") {
		echo "NamedPtr< ".$type[$i+1]." >";
		if(isset($field->def)) $field->def= '"'.$field->def.'"';
	} elseif($type[$i]=="bsize") {
		generateFieldType($struct,$field,$i+1);
	} else {
		echo "#error $type[$i]";
	}
}

function generateStructBody($struct) {
	foreach($struct->fields as $field) {
		$field->name=$field->type[0];
		if(!isset($field->tag)) {
			$field->tag = $field->name;
			$len = strlen($field->name);
			if($len>0 and $field->name[$len-1]=='N') {
				$field->tag= substr($field->name, 0, $len-1);
			}
		}
		$field->serialize=true;
		array_shift($field->type);
		if($field->type[0]=='attr') {
			array_shift($field->type);
			$struct->attrs[]=$field;
		}
		elseif($field->type[0]=='content') {
			array_shift($field->type);
			$field->def='';
			$struct->content=$field;
		}
		elseif($field->type[0]=='tagname') {
			array_shift($field->type);
			$struct->tagname=$field;
		} elseif($field->type[0]=='transient') {
			array_shift($field->type);
			$field->serialize=false;
		} elseif($field->type[0]=='ignore' or $field->type[0]=='skip') {
			$field->serialize=false;
			$struct->tags[]=$field;
		} else {
			$struct->tags[]=$field;
		}
		if($field->type[0]=='skip') $field->type[0]='ignore';
		if($field->type[0]=='ignore') {$field->serialize=false;}
		ob_start();
		echo "  ";
		generateFieldType($struct,$field,0);
		echo " $field->name;\n";
		if(!isset($field->def) and !isset($field->opt) and $field->serialize) {
			$field->optx=$struct->flags;
			$struct->flags++;
			//todo: do not for structs?
		}
		if($field->name=='scheduler_request') {
			var_dump($field);
		}
		if($field->serialize)
			ob_end_flush();
			else ob_end_clean();
		if(isset($field->opt)) {
			//echo "  bool {$field->opt};\n";
			$struct->optflags[]=$field->opt;
		}
		if(isset($field->def)) {
			if($field->def==='""') $field->def= '.clear()';
			if(is_numeric($field->def) or $field->def[0]!='.') $field->def= '= '.$field->def;
		}
	}
	foreach($struct->optflags as $field) {
		echo "  bool {$field};\n";
	}
	if($struct->enums) {
		$struct->enums=array_keys($struct->enums);
		sort($struct->enums);
		echo "  enum { ";
		foreach($struct->enums as $v) {
			echo "v_$v, ";
		} echo "};\n";
		echo "  static const char* enum_table [".sizeof($struct->enums)."];\n";
	}
	if($struct->tags) {
		usort($struct->tags,"compare_by_tag_str");
		echo "  static const char* tag_table [".sizeof($struct->tags)."];\n";
	}
	if($struct->attrs) {
		usort($struct->attrs,"compare_by_tag_str");
		echo "  static const char* attr_table [".sizeof($struct->attrs)."];\n";
	}
	//var_dump($struct);
}

function generateTables($struct) {
	if($struct->tags) {
		echo "const char* ".$struct->name."::tag_table [".sizeof($struct->tags)."] = {\n";
		foreach($struct->tags as $row) {
			echo " \"".$row->tag."\",\n";
		}
		echo "};\n\n";
	}
	if($struct->attrs) {
		echo "const char* ".$struct->name."::attr_table [".sizeof($struct->attrs)."] = {\n";
		foreach($struct->attrs as $row) {
			echo " \"".$row->tag."\",\n";
		}
		echo "};\n\n";
	}
	if($struct->enums) {
		echo "const char* ".$struct->name."::enum_table [".sizeof($struct->enums)."] = {\n";
		foreach($struct->enums as $row) {
			echo " \"".$row."\",\n";
		}
		echo "};\n\n";
	}
}

function compare_by_tag_str($l,$r) {
	return strcmp($l->tag,$r->tag);
}


function generateVectorFieldParse($ref,$field,$i)
{
	$tab = "\t\t\t";
	if(isset($field->innertag)):
?>
		while(xp.get_tag()) {
<?php if($field->innertag!='*'): ?>
			if(strcmp(xp.tag,"<?=$field->innertag?>")) continue;
<?php endif; 
		if(!empty($field->vmax))
			echo $tab."if( $ref.size() >= {$field->vmax} ) throw EXmlParse(xp, xp.array_too_long);\n";
?>
			<?=$ref?>.emplace_back();
<?php generateFieldParse("$ref.back()",$field,$i); ?>
		}
<?php	else:
		if(!empty($field->vmax))
			echo $tab."if( $ref.size() >= {$field->vmax} ) throw EXmlParse(xp, xp.array_too_long);\n";
		echo $tab."$ref.emplace_back();\n";
		generateFieldParse("$ref.back()",$field,$i);
	endif;
}
	

function generateFieldParse($ref,$field,$i)
{
	$tab = "\t\t\t";

	$flag=false;
	if(isset($field->optx)) $flag="flags[{$field->optx}]";
	if(isset($field->opt))  $flag=$field->opt;
	if($flag && $i==0) {
		echo $tab."if($flag) throw EXmlParse(xp,xp.duplicate_field);\n";
		echo $tab.$flag."=1;\n";
	}
	if($i==0 && !$flag && isset($field->def))
	{
		//todo throw duplicate with defaults
		//echo $tab."if($ref!={$field->def}) throw EXmlParse(xp,xp.duplicate_field);\n";
	}

	if($field->type[$i]=='varchar') {
		echo $tab."xp.get_str($ref, ".($field->type[$i+1]+1).");\n";
	}
	else if($field->type[$i]=='id' or $field->type[$i]=='int' or $field->type[$i]=='long') {
		echo $tab."$ref= xp.get_long();\n";
	}
	else if($field->type[$i]=='vector') {
		generateVectorFieldParse($ref,$field,$i+1);
	}
	else if($field->type[$i]=='ignore') {
		echo $tab."xp.skip();\n";
	}
	else if($field->type[$i]=='bool') {
		echo $tab."$ref= xp.get_bool();\n";
	}
	else if($field->type[$i]=='string') {
		echo $tab."xp.get_string($ref, ".($field->type[$i+1]+1).");\n";
	}
	else if($field->type[$i]=='float' or $field->type[$i]=='double') {
		echo $tab."$ref= xp.get_double();\n";
	}
	else if($field->type[$i]=='struct' or $field->type[$i]=='structn') {
		echo $tab."$ref.parse(xp);\n";
	}
	else if($field->type[$i]=='boolp') {
		//echo $tab."$ref=true;\n";
		echo $tab."xp.skip();\n";
	}
	else if($field->type[$i]=='enum') {
		echo $tab."$ref= xp.get_enum_value(enum_table,sizeof(enum_table));\n";
	}
	else if($field->type[$i]=='ulong') {
		echo $tab."$ref= xp.get_ulong();\n";
	}
	else if($field->type[$i]=='uquad') {
		echo $tab."$ref= xp.get_uquad();\n";
	}
	else if($field->type[$i]=='halt') {
		//echo $tab."$ref= true;\n";
		echo $tab."xp.halt();\n";
		echo $tab."return;\n";
	}
	else if($field->type[$i]=='named') {
		echo $tab."xp.get_str($ref.name, ".(16).");\n";
	}
	else if($field->type[$i]=='bsize') {
		echo $tab."$ref= xp.get_bsize_uquad();\n";
	}
	else echo $tab."#error parse $field->name {$field->type[$i]} :)\n";
}

function generateTagAttrParse($struct,$tag)
{
	$what=($tag?"tag":"attr");
	$list=$tag? $struct->tags : $struct->attrs;
	$table = $struct->name."::".$what."_table";
?>
	while(xp.get_<?=$what?>()) {
		long ix = xp.lookup(<?=$table?>,sizeof <?=$table?>, xp.<?=$what?>);
		//printf("<?=$what?>: %s n:%ld\n",xp.<?=$what?>,ix);
		switch(ix) {
<?php foreach($list as $index=>$field): ?>
		case <?=$index?>: //<?=$field->name?>/
<?php generateFieldParse($field->name,$field,0); ?>
			break;
<?php endforeach; if($tag): ?>
		default:
			xp.skip();
<?php else: ?>
		default:
			throw EXmlParse(xp,xp.unknown_field);
<?php endif; ?>
		}
	}
<?php
	// flags
	foreach($list as $index=>$field) if(isset($field->optx)) {
		echo "	if(!flags[{$field->optx}]) throw EXmlParse(xp,".($tag?'false':'true').",{$table}[$index]);\n";
	}

}

function generateDeserFunction($struct)
{
	echo "void {$struct->name}::parse(XML_PARSER2& xp)\n";
	echo "{\n";
	// flags
	if($struct->flags) {
		echo "	std::bitset<{$struct->flags}> flags;\n";
	}
	foreach($struct->optflags as $opt) {
		echo "	$opt = 0;\n";
	}
	// default values
	foreach($struct->fields as $field) if(isset($field->def)) {
		echo "	{$field->name}{$field->def};\n";
	}
	// tag name
	if($struct->tagname) {
		echo "	".$struct->tagname->name." = xp.tag;\n";
		//echo "	//generateTagNameParse({$struct->name});\n";
	}
	// attributes
	if($struct->attrs) {
		generateTagAttrParse($struct,false);
	}
	// contents
	if($struct->content) {
		generateFieldParse($struct->content->name,$struct->content,0);
	}
	// tags
	else if($struct->tags) {
		generateTagAttrParse($struct,true);
	} else {
		echo "	xp.skip();\n";
	}
	echo "}\n\n";
}

function processStruct($struct,$cpp,$hpp)
{
	ob_start();
	echo "struct {$struct->name} {\n";
	generateStructBody($struct);
	if($hpp) {
		echo "  void parse(XML_PARSER2& xp);\n};\n\n";
		fwrite($hpp,ob_get_clean());
	}
	else ob_end_clean();
	if($cpp) {
		ob_start();
		generateTables($struct);
		generateDeserFunction($struct);
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
	$struct= new Struct();
	while(($line = fgets($fh))!==FALSE) {
		$el=array();
		if(!preg_match("/^[[:space:]]*([^[:space:]]+)[[:space:]]+(.+)/",$line,$el)) continue;
		if($el[1][0]=='#') continue;
		if($el[1]=='hpp') {
			open_output_file($hpp,'hpp',$el[2]);
			if($hpp) fwrite($hpp, "#pragma once\n");
			//if($hpp) fwrite($hpp, "#include \"../parse2.hpp\"\n");
			//if($cpp) fwrite($cpp, "#include \"".basename($el[2])."\"\n");
		}
		elseif($el[1]=='cpp') {
			open_output_file($cpp,'cpp',$el[2]);
		}
		elseif($el[1]=='echocpp') {
			if($cpp) fwrite($cpp, $el[2]."\n");
		}
		elseif($el[1]=='echohpp') {
			if($hpp) fwrite($hpp, $el[2]."\n");
		}
		elseif($el[1]=='struct') {
			$struct->name= $el[2];
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
				$rline=new stdClass();
				$rline->type=array();
				foreach( explode(" ",$line) as $word) {
					if($word=="") continue;
					$kv=explode("=",$word);
					if(sizeof($kv)==2) {
						$rline->{$kv[0]}=$kv[1];
					} else {
						$rline->type[]=$word;
					}
				}
				$struct->fields[]=$rline;
			}
			processStruct($struct,$cpp,$hpp);
			$struct= new Struct();
		} else {
			echo "Unrecognized line $line\n";
			//var_dump($el);
		}
	}
	if($cpp) fclose($cpp);
	if($hpp) fclose($hpp);
}

if(!empty($argv[1])) {
	processInputFile($argv[1]);
}
