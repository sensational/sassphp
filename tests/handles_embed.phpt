--TEST--
correctly handles setting and getting source embedding
--SKIPIF--
<?php if (!extension_loaded("sass")) print "skip"; ?>
--FILE--
<?php

$sass = new Sass();
// test default from constructor
echo $sass->getComments();

// test SASS_SOURCE_NONE
$sass->setEmbed(false);
echo $sass->getEmbed();

// test SASS_SOURCE_DEFAULT
$sass->setEmbed(true);
echo $sass->getEmbed();


?>
--EXPECT--
001
