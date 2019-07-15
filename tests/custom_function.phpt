--TEST--
custom importer: external file
--SKIPIF--
<?php if (!extension_loaded("sass")) print "skip"; ?>
--FILE--
<?php

$sass = new Sass();
$sass->setFunctions([
    'a($a)' => function($in, $path_info){
        echo $in;
        return "hello $in";
    },
    'b($a)' => function($in, $path_info){
        echo $in;
        return "goodbye $in";
    },
]);
echo $sass->compile('body { content: a("foo"); } h1 { content: b("bar"); }');

?>
--EXPECT--
foobarbody {
  content: hello foo; }

h1 {
  content: goodbye bar; }
