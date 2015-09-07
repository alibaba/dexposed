<?php

/**
*  run in src/main/jni/include dir to copy header file from android souces dir
*  php $this_script_path $android_source_path $jni_file_path
*  etc:
*     cd dexposed_dalivik/src/main/jni/include
*     php /Volumes/d/dexposed/tools/extractAndroidSourceHeader.php /Volumes/d/android-source/ ../dexposed.cpp
*/

require_once("./search_dir.php");

$pwd=`pwd`;
$aospDir=$argv[1]."/";

for($i=0;$i<count($searchDir);$i++)
  $searchDir[$i]=$aospDir.$searchDir[$i];

$result=parseOne($argv[2]);

function findHeaderInAospPath($includeFile)
{
  global $searchDir,$aospDir;

  $grep = substr($includeFile,0,-2);
  $grep = str_replace("../","",$grep);
  $cmd = "find ".implode(" ",$searchDir)." |grep \"/$grep\.h\"";
  $path = `$cmd`;
  if(null === $path)
  {
    return null;
  }
  else {
    $arr = explode("\n",$path);
    return $arr[0];
  }

}

function parseOne($file,$alreadyInclude=array())
{
  global $searchDir,$aospDir;

  $alreadyInclude[$file]=$file;

  $fd=fopen($file,"r");
  if($fd === False)
  {
    echo "===============open file $file error";
    exit;
  }

  while(($line = fgets($fd)) !==false )
  {
    $line=trim($line);
    if(preg_match("/#include\s*[<\"](.+\.h)[\">]/",$line,$matches))
    {
      $includeFile = $matches[1];
      $inAospPath = findHeaderInAospPath($includeFile);
      if(null === $inAospPath){
        echo "$includeFile no found\n";
        continue;
      }

      if(!isset($alreadyInclude[$inAospPath]))
      {
        $savePath=str_replace($aospDir,"",$inAospPath);
        $saveDir=dirname($savePath);
        `mkdir -p ./$saveDir`;
        `cp $inAospPath $savePath`;

        $tempArr = parseOne($inAospPath,$alreadyInclude);
        $alreadyInclude = array_merge($alreadyInclude,$tempArr);
      }else{
      }

    }
  }

  fclose($fd);
  return $alreadyInclude;
}
