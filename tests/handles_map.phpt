--TEST--
correctly handles setting and getting source map formats
--SKIPIF--
<?php if (!extension_loaded("sass")) print "skip"; ?>
--FILE--
<?php

$sass = new Sass();
// test default from constructor
$sass->setComments(true);
$sass->setMapPath(__DIR__.'/support/huge.css.map');
$sass->compileFile(__DIR__.'/support/test.scss');

?>
--EXPECT_EXTERNAL--
support/huge.css.map
--EXPECT--

