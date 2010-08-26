# 
#     Copyright 2010, Kay Hayen, mailto:kayhayen@gmx.de
# 
#     Part of "Nuitka", my attempt of building an optimizing Python compiler
#     that is compatible and integrates with CPython, but also works on its
#     own.
# 
#     If you submit patches to this software in either form, you automatically
#     grant me a copyright assignment to the code, or in the alternative a BSD
#     license to the code, should your jurisdiction prevent this. This is to
#     reserve my ability to re-license the code at any time.
# 
#     This program is free software: you can redistribute it and/or modify
#     it under the terms of the GNU General Public License as published by
#     the Free Software Foundation, version 3 of the License.
# 
#     This program is distributed in the hope that it will be useful,
#     but WITHOUT ANY WARRANTY; without even the implied warranty of
#     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#     GNU General Public License for more details.
# 
#     You should have received a copy of the GNU General Public License
#     along with this program.  If not, see <http://www.gnu.org/licenses/>.
# 
import hashlib, marshal, re

class Identifier:
    def __init__( self, code, ref_count ):
        self.code = code
        self.ref_count = ref_count

    def getRefCount( self ):
        return self.ref_count

    def setRefCount( self, ref_count ):
        assert hasattr( self, "ref_count" )
        self.ref_count = ref_count

    def getCode( self ):
        return self.code

    def getCodeObject( self ):
        return self.getCode()

    def getCodeExportRef( self ):
        if self.getRefCount():
            self.ref_count = 0

            return self.code
        else:
            return "INCREASE_REFCOUNT( %s )" % self.getCodeObject()

    def getCodeTemporaryRef( self ):
        if self.getRefCount():
            return "PyObjectTemporary( %s ).asObject()" % self.getCode()
        else:
            return self.getCodeObject()

    def getCodeDropRef( self ):
        if self.ref_count == 0:
            return self.code
        else:
            return "DECREASE_REFCOUNT( %s )" % self.getCodeObject()

    def __repr__( self ):
        return "<Identifier %s (%d)>" % ( self.code, self.ref_count )

class LocalVariableIdentifier( Identifier ):
    def __init__( self, var_name, from_context = False ):
        assert type( var_name ) == str

        self.from_context = from_context
        self.var_name = var_name

    def __repr__( self ):
        return "<LocalVariableIdentifier %s>" % self.var_name

    def getCode( self ):
        if not self.from_context:
            return "_python_var_" + self.var_name
        else:
            return "_python_context->python_var_" + self.var_name

    def getRefCount( self ):
        return 0

    def getCodeObject( self ):
        return "%s.asObject()" % self.getCode()


class LocalLoopVariableIdentifier( Identifier ):
    def __init__( self, loopvar_name ):
        assert type( loopvar_name ) == str

        self.loopvar_name = loopvar_name

    def __repr__( self ):
        return "<LocalLoopVariableIdentifier %s >" % self.loopvar_name

    def getCode( self ):
        return "_python_var_" + self.loopvar_name

    def getRefCount( self ):
        return 0

    def getCodeObject( self ):
        return "%s.asObject()" % self.getCode()

class TempVariableIdentifier( Identifier ):
    def __init__( self, tempvar_name ):
        self.tempvar_name = tempvar_name

    def __repr__( self ):
        return "<TempVariableIdentifier %s >" % self.tempvar_name

    def getRefCount( self ):
        return 0

    def getCode( self ):
        return "_python_tmp_" + self.tempvar_name

    def getCodeObject( self ):
        return self.getCode() + ".asObject()"


class ClosureVariableIdentifier( Identifier ):
    def __init__( self, var_name, from_context ):
        assert type( from_context ) == str

        self.var_name = var_name
        self.from_context = from_context

    def __repr__( self ):
        return "<ClosureVariableIdentifier %s >" % self.var_name

    def getCode( self ):
        if self.from_context:
            return self.from_context + "python_closure_" + self.var_name
        else:
            return "_python_closure_" + self.var_name

    def getRefCount( self ):
        return 0

    def getCodeObject( self ):
        return self.getCode() + ".asObject()"

    def getCodeDropRef( self ):
        return "DECREASE_REFCOUNT( %s )" % self.getCodeObject()

class ExceptionCannotNamify( Exception ):
    pass

def digest( value ):
    return hashlib.md5( value ).hexdigest()

_re_str_needs_no_digest = re.compile( r"^([a-z]|[A-Z]|[0-9]|_){1,40}$", re.S )

def _namifyString( string ):
    if string == "":
        return "empty"
    elif _re_str_needs_no_digest.match( string ) and "\n" not in string:
        # Some strings can be left intact for source code readability.
        return "plain_" + string
    elif len( string ) > 2 and string[0] == "<" and string[-1] == ">" and _re_str_needs_no_digest.match( string[1:-1] ) and "\n" not in string:
        return "angle_" + string[1:-1]
    else:
        # Others are better digested to not cause compiler trouble
        return "digest_" + digest( string )

def isAscii( string ):
    try:
        _unused = str( string )

        return True
    except UnicodeEncodeError:
        return False

def namifyConstant( constant ):
    if type( constant ) == int:
        if constant == 0:
            return "int_0"
        elif constant > 0:
            return "int_pos_%d" % constant
        else:
            return "int_neg_%d" % abs( constant )
    elif type( constant ) == long:
        if constant == 0:
            return "long_0"
        elif constant > 0:
            return "long_pos_%d" % constant
        else:
            return "long_neg_%d" % abs( constant )
    elif type( constant ) == bool:
        return "bool_%s" % constant
    elif type( constant ) == str:
        return "str_" + _namifyString( constant )
    elif type( constant ) == unicode:
        if isAscii( constant ):
            return "unicode_" + _namifyString( str( constant ) )
        else:
            # Others are better digested to not cause compiler trouble
            return "unicode_digest_" + digest( repr( constant ) )
    elif type( constant ) == float:
        return "float_%s" % repr( constant ).replace( ".", "_" ).replace( "-", "_minus_" ).replace( "+", "" )
    elif type( constant ) == complex:
            return "complex_%s" % str( constant ).replace( "+", "p" ).replace( "-", "m" ).replace(".","_")
    else:
        raise ExceptionCannotNamify( constant )

if __name__ == "__main__":
    for value in ( "", "<module>" ):
        print value, ":", namifyConstant( value )
