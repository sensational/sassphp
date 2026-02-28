<?

/**
 * SASS compiler via libsass
 * @author Sergey Odintsov <nixx.dj@gmail.com>
 * @link https://github.com/sensational/sassphp
 */
class Sass {

	/**
	 * 'Nested' format
	 * */
	const STYLE_NESTED = 0;

	/**
	 * 'Expanded' format
	 */
	const STYLE_EXPANDED  = 1;

	/**
	 * 'Compact' format
	 */
	const STYLE_COMPACT = 2;

	/**
	 * 'Compressed' format
	 */
	const STYLE_COMPRESSED = 3;

	/**
	 * Constructor SASS
	 */
	public function __construct() {}

	/**
	 * Compile string
	 * @param string $string input SASS string
	 * @return string output CSS
	 * @throws SassException if compilation failed
	 */
	public function compile($string) {}

	/**
	 * Compile file
	 * @param string $filename input filename
	 * @return string compiled content of the file
	 * @throws SassException if compilation failed or invalid arguments given
	 */
	public function compileFile($filename) {}

	/**
	 * Set style
	 * @param int $style
	 * @return void
	 */
	public function setStyle($style) {}

	/**
	 * Get style
	 * @return int
	 */
	public function getStyle() {}

	/**
	 * Set enable comments
	 * @param bool $comments
	 * @return bool
	 */
	public function setComments($comments) {}

	/**
	 * Is enable comments
	 * @return bool
	 */
	public function getComments() {}

	/**
	 * Set map path
	 * @param string $map input path
	 * @return void
	 */
	public function setMapPath($map) {}

	/**
	 * Get map path
	 * @return string
	 */
	public function getMapPath($map) {}

	/**
	 * Set include path
	 * @param string $path input directory
	 * @return void
	 */
	public function setIncludePath($path) {}

	/**
	 * Get include path
	 * @return string
	 */
	public function getIncludePath() {}

	/**
	 * Set precision
	 * @param int $precision
	 * @return void
	 */
	public function setPrecision($precision) {}

	/**
	 * Get include path
	 * @return int
	 */
	public function getPrecision() {}

	/**
	 * Set map embed
	 * @param bool $embed
	 * @return void
	 */
	public function setEmbed($embed) {}

	/**
	 * Is map embed
	 * @return bool
	 */
	public function getEmbed() {}
}

/**
 * Runtime compilation error
 */
class SassException extends Exception {}