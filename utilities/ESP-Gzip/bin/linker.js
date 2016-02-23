/**
 * \brief           ESP-Gzip Utility
 * \description     Generates embeddable C files of webpages located in the source folder
 * \file            linker.js
 * \author          Cristian Dobre
 * \version 	    1.0.1
 * \date 		    February 2016
 * \copyright 	    Revised BSD License.
 */


/**
 * Path from current location to the source directory
 */
var sourcePath = "../source/";
var minifyPath = "../minified/";
var compressPath = "../compressed/";
var webcontentPath = "../webcontent/";

function getResources() {
    return { 
        html:   getDirectoryFiles( sourcePath, "", [ "htm", "html" ] ), 
        js:     getDirectoryFiles( sourcePath, "js/", [ "js" ] ), 
        css:    getDirectoryFiles( sourcePath, "css/", [ "css" ] )
    };
}


function getDirectoryFiles( path, directory, filetype )
{
    var fs = require( 'fs' ), files = [], r, resource;
    try {
        resource = fs.lstatSync( path + directory ); 
        if( resource.isDirectory() ) {
            resource = fs.readdirSync( path + directory );
            for( var i in resource ) {
                if( ! fs.statSync( path + directory + resource[ i ] ).isDirectory() ) {
                    if( filetype != undefined ) {
                        if( filetype.indexOf( ( r = resource[ i ].split( '.' ))[ r.length - 1 ] ) < 0 )
                            continue;
                    }
                    files.push( directory + resource[ i ] );
                }
            }
        }
    } catch ( e ) {}
    return files;
}


function cleanDirectory( path, directory )
{
    var fs = require( 'fs' ), files = [], r, resource;
    try {
        resource = fs.lstatSync( path + directory ); 
        if( resource.isDirectory() ) {
            resource = fs.readdirSync( path + directory );
            for( var i in resource )
                if( ! fs.statSync( path + directory + resource[ i ] ).isDirectory() )
                    fs.unlinkSync( path + directory + resource[ i ] );
        }
    } catch ( e ) {}
    return files;
}



function minifyResources( resources )
{
    var minifyHTML = require( "html-minifier" ), minifyCSS = require( "clean-css" ), minifyJS = require( "uglify-js" ), fs = require( 'fs' );
    var shell = require( "child_process" ), compress = "7za a ", compress_cmd = [];
    var file, html = resources.html, css = resources.css, js = resources.js, minified, fname;
    var script_regex = /<script(.+?)>(\s)*?<\/script>/g, match, attr, location, href, aux, index;
    var css_regex = /<link(.+)>/g
    var generate_compiled = [];
    var generate_cfiles = [], cfile, cfname, c_code;
    
    cleanDirectory( minifyPath, "" );
    cleanDirectory( minifyPath, "css/" );
    cleanDirectory( minifyPath, "js/" );
    cleanDirectory( compressPath, "" );
    cleanDirectory( webcontentPath, "" );
    
    generate_compiled = generate_compiled.concat( resources.html ).concat( resources.css ).concat( resources.js );
    location = [];
    for( var i in html ) {
        fname = html[ i ];
        
        file = fs.readFileSync( sourcePath + fname, 'utf8' );
        file = minifyHTML.minify( file, {
            minifyCSS: true,
            minifyJS: true,
			preserveLineBreaks: false			
        });
        match = file.match( css_regex );
		if( match )	
			for( var i = 0 ; i < match.length ; i++ ) {
				href = null;
				attr = match[ i ].match( /href="((\S)+)"/ );
				if( attr ) href = attr[ 1 ];
				else {
					match[ i ].match( /href='((\S)+)'/ );
					if( attr ) href = attr[ 1 ];
				}
				if( href ) {
					if( href.match( "#inline" ) ) {
						href = href.replace( "#inline", "" );
						if( resources.css.indexOf( href ) >= 0 ) {
							if( ( index = generate_compiled.indexOf( href )) >= 0 ) 
								generate_compiled.splice( index, 1 );
							minified = new minifyCSS().minify( fs.readFileSync( sourcePath + css[ i ], 'utf8' ) );
							file = file.replace( match[ i ], "<style>" + minified.styles + "</style>" );
						}
					} else if( resources.css.indexOf( href ) >= 0 ) {
						aux = href.split( '.' );
						if( aux[ aux.length - 1 ] == "css" ) aux[ aux.length - 1 ] = "min." + aux[ aux.length - 1 ];
						aux = match[ i ].replace( href, aux.join( '.' ) );
						file = file.replace( match[ i ], aux );
					}
				}
			}
        
        match = file.match( script_regex );
        if( match )
			for( var i = 0 ; i < match.length ; i++ ) {
				href = null;
				attr = match[ i ].match( /src="((\S)+)"/ );
				if( attr ) href = attr[ 1 ];
				else {
					match[ i ].match( /src='((\S)+)'/ );
					if( attr ) href = attr[ 1 ];
				}
				if( href ) {
					if( href.match( "#inline" ) != null ) {
						href = href.replace( "#inline", "" );
						if( resources.js.indexOf( href ) >= 0 ) {
							if( ( index = generate_compiled.indexOf( href )) >= 0 ) 
								generate_compiled.splice( index, 1 );
							minified = minifyJS.minify( sourcePath + href );
							file = file.split( match[ i ] );
							file = file[ 0 ] + "<script>" + minified.code + "</script>" + file[ 1 ];
						}
					} else if( resources.js.indexOf( href ) >= 0 ) {
						aux = href.split( '.' );
						if( aux[ aux.length - 1 ] == "js" ) aux[ aux.length - 1 ] = "min." + aux[ aux.length - 1 ];
						aux = match[ i ].replace( href, aux.join( '.' ) );
						file = file.replace( match[ i ], aux );
					}
				}
			}
        
        fs.writeFileSync( minifyPath + fname, file, 'ascii' );      
        compress_cmd.push( compress + ( compressPath + fname + '.gz' ) + " -tgzip -si < " + ( minifyPath + fname ) );
        generate_cfiles.push( compressPath + fname + '.gz' );
    }
    
    for( var i in css ) {
        if( generate_compiled.indexOf( css[ i ] ) >= 0 ) {
            fname = css[ i ].split( '.' );
            fname[ 0 ] += '.min'; fname = fname.join( '.' );

            file = fs.readFileSync( sourcePath + css[ i ], 'ascii' );
            minified = new minifyCSS().minify( file ).styles;
            fs.writeFileSync( minifyPath + fname, minified, 'ascii' );

            compress_cmd.push( compress + ( compressPath + "css_" + css[ i ].replace( "css/", "" ) + '.gz' ) + " -tgzip -si < " + ( minifyPath + fname ) );
            generate_cfiles.push( compressPath + "css_" + css[ i ].replace( "css/", "" ) + '.gz' );
        }
    }
    
    for( var i in js ) {
        if( generate_compiled.indexOf( js[ i ] ) >= 0 ) {
            fname = js[ i ].split( '.' );
            fname[ 0 ] += '.min'; fname = fname.join( '.' );

            minified = minifyJS.minify( sourcePath + js[ i ] );
            fs.writeFileSync( minifyPath + fname, minified.code, 'ascii' );

            compress_cmd.push( compress + ( compressPath + "js_" + js[ i ].replace( "js/", "" ) + '.gz' ) + " -tgzip -si < " + ( minifyPath + fname ) );
            generate_cfiles.push( compressPath + "js_" + js[ i ].replace( "js/", "" ) + '.gz' );
        }
    }
 
    for( var i in compress_cmd ) {
		shell.execSync( compress_cmd[ i ] );        
    }
    
    for( var i in generate_cfiles ) {
        file = fs.readFileSync( generate_cfiles[ i ], 'ascii' );
        cfname = generate_cfiles[ i ].replace( '.gz', '' ).replace( '../compressed/', '' );
        cfile = "\
/**\r\n\
 * \\brief           ESP-Gzip Compiled Webpage \r\n\
 * \\description     Don't forget to include the appropriate compression headers in your HTTP response \r\n\
 * \\source          " + cfname + " \r\n\
 * \\generated:      " + ( new Date() ).toLocaleString() + " \r\n\
 */\r\n";
        cfname = 'webcontent_' + cfname.replace( '.', '_' );
        cfile += "#define " + cfname.toUpperCase() + "_LENGTH    " + file.length + "\r\n\r\n";
        cfile += "static char " + cfname + "[ " + file.length + " ] = { ";
        
        for( var j = 0; j < file.length ; j++ ) {
            c_code = file.charCodeAt( j );
            cfile += "0x" + ( ( "00" + c_code.toString( 16 ) ).substr( -2 ) );
            if( file.length - 1 != j ) cfile += ", ";
        }
        cfile += " }; \r\n";
        
        fs.writeFileSync( webcontentPath + cfname + '.c', cfile, 'ascii' );
    }    
    
}


minifyResources( getResources() );