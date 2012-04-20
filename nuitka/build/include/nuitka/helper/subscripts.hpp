//     Copyright 2012, Kay Hayen, mailto:kayhayen@gmx.de
//
//     Part of "Nuitka", an optimizing Python compiler that is compatible and
//     integrates with CPython, but also works on its own.
//
//     If you submit patches or make the software available to licensors of
//     this software in either form, you automatically them grant them a
//     license for your part of the code under "Apache License 2.0" unless you
//     choose to remove this notice.
//
//     Kay Hayen uses the right to license his code under only GPL version 3,
//     to discourage a fork of Nuitka before it is "finished". He will later
//     make a new "Nuitka" release fully under "Apache License 2.0".
//
//     This program is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, version 3 of the License.
//
//     This program is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//     Please leave the whole of this copyright notice intact.
//
#ifndef __NUITKA_HELPER_SUBSCRIPTS_H__
#define __NUITKA_HELPER_SUBSCRIPTS_H__

extern PyObject *BUILTIN_CHR( unsigned char c );

NUITKA_MAY_BE_UNUSED static PyObject *LOOKUP_SUBSCRIPT_CONST( PyObject *source, PyObject *const_subscript, Py_ssize_t int_subscript )
{
    assertObject( source );
    assertObject( const_subscript );

    PyTypeObject *type = Py_TYPE( source );
    PyMappingMethods *m = type->tp_as_mapping;

    PyObject *result;

    if ( m && m->mp_subscript )
    {
        if ( PyList_CheckExact( source ) )
        {
            Py_ssize_t list_size = PyList_GET_SIZE( source );

            if ( int_subscript < 0 )
            {
                if ( -int_subscript > list_size )
                {
                    PyErr_Format( PyExc_IndexError, "list index out of range" );
                    throw _PythonException();
                }

                int_subscript += list_size;
            }
            else
            {
                if ( int_subscript >= list_size )
                {
                    PyErr_Format( PyExc_IndexError, "list index out of range" );
                    throw _PythonException();
                }
            }

            return INCREASE_REFCOUNT( ((PyListObject *)source)->ob_item[ int_subscript ] );
        }
#if PYTHON_VERSION < 300
        // TODO: May also be useful for Python3.
        else if ( PyString_CheckExact( source ) )
        {
            Py_ssize_t string_size = PyString_GET_SIZE( source );

            if ( int_subscript < 0 )
            {
                if ( -int_subscript > string_size )
                {
                    PyErr_Format( PyExc_IndexError, "string index out of range" );
                    throw _PythonException();
                }

                int_subscript += string_size;
            }
            else
            {
                if ( int_subscript >= string_size )
                {
                    PyErr_Format( PyExc_IndexError, "string index out of range" );
                    throw _PythonException();
                }
            }

            unsigned char c = ((PyStringObject *)source)->ob_sval[ int_subscript ];
            return BUILTIN_CHR( c );
        }
#endif
        else
        {
            result = m->mp_subscript( source, const_subscript );
        }
    }
    else if ( type->tp_as_sequence )
    {
        result = PySequence_GetItem( source, int_subscript );
    }
    else
    {
        return PyErr_Format( PyExc_TypeError, "'%s' object is unsubscriptable", Py_TYPE( source )->tp_name );
        throw _PythonException();
    }

    if (unlikely( result == NULL ))
    {
        throw _PythonException();
    }

    return result;
}

#define LOOKUP_SUBSCRIPT( source, subscript ) _LOOKUP_SUBSCRIPT( EVAL_ORDERED_2( source, subscript ) )

NUITKA_MAY_BE_UNUSED static PyObject *_LOOKUP_SUBSCRIPT( EVAL_ORDERED_2( PyObject *source, PyObject *subscript ) )
{
    assertObject( source );
    assertObject( subscript );

    PyTypeObject *type = Py_TYPE( source );
    PyMappingMethods *mapping = type->tp_as_mapping;

    PyObject *result;

    if ( mapping != NULL && mapping->mp_subscript != NULL )
    {
        result = mapping->mp_subscript( source, subscript );
    }
    else if ( type->tp_as_sequence != NULL )
    {
        if ( PyIndex_Check( subscript ) )
        {
            result = PySequence_GetItem( source, CONVERT_TO_INDEX( subscript ) );
        }
        else if ( type->tp_as_sequence->sq_item )
        {
            PyErr_Format( PyExc_TypeError, "sequence index must be integer, not '%s'", Py_TYPE( subscript )->tp_name );
            throw _PythonException();
        }
        else
        {
            return PyErr_Format( PyExc_TypeError, "'%s' object is unsubscriptable", Py_TYPE( source )->tp_name );
            throw _PythonException();
        }
    }
    else
    {
        return PyErr_Format( PyExc_TypeError, "'%s' object is unsubscriptable", Py_TYPE( source )->tp_name );
        throw _PythonException();
    }

    if (unlikely( result == NULL ))
    {
        throw _PythonException();
    }

    return result;
}

#define SET_SUBSCRIPT( value, target, subscript ) _SET_SUBSCRIPT( EVAL_ORDERED_3( value, target, subscript ) )

NUITKA_MAY_BE_UNUSED static void _SET_SUBSCRIPT( EVAL_ORDERED_3( PyObject *value, PyObject *target, PyObject *subscript ) )
{
    assertObject( value );
    assertObject( target );
    assertObject( subscript );

    PyMappingMethods *mapping_methods = Py_TYPE( target )->tp_as_mapping;

    if ( mapping_methods != NULL && mapping_methods->mp_ass_subscript )
    {
        int res = mapping_methods->mp_ass_subscript( target, subscript, value );

        if (unlikely( res == -1 ))
        {
            throw _PythonException();
        }
    }
    else if ( Py_TYPE( target )->tp_as_sequence )
    {
        if ( PyIndex_Check( subscript ) )
        {
            Py_ssize_t key_value = PyNumber_AsSsize_t( subscript, PyExc_IndexError );

            if ( key_value == -1 )
            {
                THROW_IF_ERROR_OCCURED();
            }

            SEQUENCE_SETITEM( target, key_value, value );

        }
        else if ( Py_TYPE( target )->tp_as_sequence->sq_ass_item )
        {
            PyErr_Format(
                PyExc_TypeError,
                "sequence index must be integer, not '%s'",
                Py_TYPE( subscript )->tp_name
            );

            throw _PythonException();
        }
        else
        {
            PyErr_Format(
                PyExc_TypeError,
                "'%s' object does not support item assignment",
                Py_TYPE( target )->tp_name
            );

            throw _PythonException();
        }
    }
    else
    {
        PyErr_Format(
            PyExc_TypeError,
            "'%s' object does not support item assignment",
            Py_TYPE( target )->tp_name
        );

        throw _PythonException();
    }
}

#define DEL_SUBSCRIPT( target, subscript ) _DEL_SUBSCRIPT( EVAL_ORDERED_2( target, subscript ) )

NUITKA_MAY_BE_UNUSED static void _DEL_SUBSCRIPT( EVAL_ORDERED_2( PyObject *target, PyObject *subscript ) )
{
    assertObject( target );
    assertObject( subscript );

    int status = PyObject_DelItem( target, subscript );

    if (unlikely( status == -1 ))
    {
        throw _PythonException();
    }
}

#endif
