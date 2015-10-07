# sassphp

The `sass` extension for PHP gives you an object-oriented system of parsing [Sass](http://sass-lang.com/) from within your PHP applications. Under the hood it uses [libsass](https://github.com/hcatlin/libsass) to provide super speedy and compatible Sass parsing.

## What's Sass?

Sass is a CSS pre-processor language to add on exciting, new, awesome features to CSS. Sass was the first language of its kind and by far the most mature and up to date codebase.

Sass was originally created by Hampton Catlin ([@hcatlin](http://twitter.com/hcatlin)). The extension and continuing evolution of the language has all been the result of years of work by Natalie Weizenbaum ([@nex4](http://twitter.com/nex4)) and Chris Eppstein ([@chriseppstein](http://twitter.com/chriseppstein)).

For more information about Sass itself, please visit [http://sass-lang.com](http://sass-lang.com)

## Installation

Currently, the only way to install the extension is manually:

    $ git clone git://github.com/sensational/sassphp

Remember to grab your submodules:

    $ git submodule init
    $ git submodule update

Also remember that libsass now uses submodules on its own, so update those too.

...and compile it! I've written a little PHP script to do all the stuff you need to do:

    $ php install.php

Run the tests:

    $ make test

Finally, you can install with `make`:

    $ make install

And then add it to your _php.ini_:

    extension=sass.so

## Usage

This extension has a very simple API:

    $sass = new Sass();
    $css = $sass->compile($source);

You can compile a file with `compileFile()`:

    $sass = new Sass();
    $css = $sass->compileFile($source);

You can set the include path for the library to use:

    $sass = new Sass();
    $sass->setIncludePath('/tmp');
    $css = $sass->compile($source);

You can set the style of your SASS file to suit your needs:

    $sass = new Sass();
    $sass->setStyle(Sass::STYLE_NESTED);

    $sass = new Sass();
    $sass->setStyle(Sass::STYLE_EXPANDED);

    $sass = new Sass();
    $sass->setStyle(Sass::STYLE_COMPACT);

    $sass = new Sass();
    $sass->setStyle(Sass::STYLE_COMPRESSED);

The new additions to this build from the [Sensational](https://github.com/sensational/sassphp) codebase are:
* Camel case rename of `compile_file` to `compileFile`
* SASS file compilation is now an array when a source map is enabled. Otherwise, as per normal, it's a string.
* The ability to define source comments
* The ability to set a source map path, required when generating a dedicated .map file

It also reports itself as the `sensational` version of the Apache SASS module to help differentiate builds.

The output of `compileFile()` is now an array instead of a string, allowing both compiled SASS file and .map file to be generated in the same function call. As there are multiple ways of generating source comments, there are now PHP level settings to control that output.

To generate source comments for a file inline - now in camelCase:

    $sass = new Sass();
    $sass->setComments(true);
    $css = $sass->compileFile($source);

Aliases also exist so you can also use:

    $css = $sass->compile_file($source);

You can set the source map file for the library to use:

    $sass = new Sass();
    $sass->setMapPath('/random.output.css.map');
    $css = $sass->compileFile($source);

This needs to be done prior to getting the output of the map file. As it stands, both the output of the SASS file compile & the SASS source map file generation sequence are both strings.

The first array item will always be the compiled SASS file:
    $css[0]

The second array item will always be the source map output:
    $css[1]

If there's a problem, the extension will throw a `SassException`:

    $sass = new Sass();

    try
    {
        $css = $sass->compile('dayrui3dui36di37');
    }
    catch (SassException $e)
    {
        // $e->getMessage() - ERROR -- , line 1: invalid top-level expression

        $css = FALSE;
    }

## Potential / known issues

* The file_comments test will fail as it remains unable to figure out the path the test has been run in. It generates the output successfully


## Forward planning

* HHVM Zend compatibility?
* Nginx extension?

## Changelog

** Version 0.4.5 
* Holiday Patch (Libsass 3.2.5) stable

** Version 0.4.4 
* Refactor correctly for LibSass 3.2.4

** Version 0.4.2
* CamelCase compile sequence

** Version 0.4.1**
* SOURCE_DEFAULT works & is documented
* PHP test for SOURCE_DEFAULT (output unable to understand source directory in test, so functional pass, technical fail)
* Revise other tests to pass as output varies on source negotiation.

** Version 0.4**
* Reconciled file compliation loop
* Expand documentation
* Addition of basic test for source comments
* More work on compile sequence

** Version 0.3.9**
* Addition of source map url paths to SASS file compilation

** Version 0.3.5**
* Restructure of compile sequence (file only at this time) to account for source map output

** Version 0.3.0**
* Addition of source comments - none, inline, file. Set default to none.

** Version 0.2.0**
* Changed methods to be non-static
* Allow setting include-path and image-path

**Version 0.1.0 - IN DEVELOPMENT**
* Initial release
