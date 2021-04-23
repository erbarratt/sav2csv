#ifndef EOOPC_MAIN_H
#define EOOPC_MAIN_H
	
#include <stdlib.h>
#include <string.h>
#include <errno.h>
	
	/**
	* Colours for the console
	*/
		#define  cRED "\x1b[31m"
		#define  cGREEN "\x1b[32m"
		#define  cYELLOW "\x1b[33m"
		#define  cBLUE "\x1b[34m"
		#define  cMAGENTA "\x1b[35m"
		#define  cCYAN "\x1b[36m"
		#define  cRESET "\x1b[0m"
	
	/**
	* Debugging macros originally taken from
	* Learn C the Hard Way: Practical Exercises on the Computational Subjects You Keep Avoiding (Like C)
	* Book by Zed Shaw (Zed's Aesome Debug Macros)
	*/

		#define eERR 1
	
		/**
		* Set up some debugging functions that can be globally turned off with a definition
		*/
			#ifdef eNODEBUG
				#define eDEBUG(msg, ...)
			#else
				#define eDEBUG(msg, ...) fprintf(stderr, "DEBUG %s:%d: " msg "\n", __FILE__, __LINE__, ##__VA_ARGS__)
			#endif
	
		/**
		* Get string version of global errno
		*/
			#define eCLEANERRNO() (errno == 0 ? "None" : strerror(errno))
		
		/**
		* Log an error
		*/
			#define eLOGERROR(msg, ...) fprintf(stderr, cRED "[ERROR] (%s:%d: errno: %s) " msg "\n" cRESET, \
			__FILE__, __LINE__, eCLEANERRNO(), ##__VA_ARGS__)
		
		/**
		* Log a warning
		*/
			#define eLOGWARN(msg, ...) fprintf(stderr, cYELLOW "[WARN] (%s:%d: errno: %s) " msg "\n" cRESET, \
			__FILE__, __LINE__, eCLEANERRNO(), ##__VA_ARGS__)
		
		/**
		* Log some info
		*/
			#define eLOGINFO(msg, ...) fprintf(stderr, cRESET "[INFO] (%s:%d) " msg "\n", __FILE__, __LINE__, ##__VA_ARGS__)
		
		/**
		* Try a test, outputting an error if false, then goto a given error_X marker
		*/
			#define eCHECK(test,err,msg, ...) if(test) { eLOGERROR(msg, ##__VA_ARGS__); errno=0; goto error_##err; } else errno=0
	
		/**
		* Mark a chunk of code that will throw an error. Usage eTRY{ ... }. "return" should be within try block
		*/
			#define eTRY if(1)
		
		/**
		* Throw an error to be "caught" by the appropriate catch marker
		*/
			#define eTHROW(exc,msg, ...) eLOGERROR(msg, ##__VA_ARGS__); goto error_##exc
	
		/**
		* wrap definition of error_x marker
		*/
			#define eCATCH(n) error_##n: if(1)
	
	/**
	* OOP Macros. Read up on OOP here:
	* https://github.com/erbarratt/eoopc
	* Extreme C: Taking you to the limit in Concurrency, OOP, and the most advanced capabilities of C - Kamran Amini
	*/
	
		/**
		* grab an interface definition by calling it's macro.
		* @param i Interface name
		*/
			#define eIMPLEMENTS(i) eINTERFACE_##i()
	
		/**
		* helper macro to denote that this parent is upcastable (this macro must be first element of containing
		* struct for this to be true)
		* @param c Class
		* @param p Property name
		*/
			#define eINHERITS(c,p) struct c p
		
		/**
		* Instantiate an object 'o*' of type 'c' by using function 'c_instatiate()'
		* @param c Struct type
		* @param o Object variable name
		* @param ... any further arguments
		*/
			#define eCONSTRUCT(c, ...) c##_instantiate(__VA_ARGS__)
		
		/**
		* Call allocation method and imediately fire instantiation function for heap object
		* @param c Struct type
		* @param o Object variable name
		* @param ... any further arguments
		*/
			#define eNEW(c) (struct c *)malloc(sizeof(struct c))
		
		/**
		* public property DECLARATION
		* @param t Type
		* @param p Property name
		*/
			#define ePROP_DEC(t, p) t p
		
		/**
		* public property DEFINITION
		* @param p Property name
		* @param v Value
		*/
			#define ePROP_DEF(p, v) self->p = v
		
		/**
		* private property PUBLIC function-pointer DECLARATIONS for public struct
		* @param t Type
		* @param p Property name
		* @param m get/set/getset
		*/
			#define ePRIV_PROP_DEC_get(t,p) t (*get_##p)(void * eOBJ)
			#define ePRIV_PROP_DEC_set(t,p) void (*set_##p)(void * eOBJ, t v)
			#define ePRIV_PROP_DEC_getset(t,p) ePRIV_PROP_DEC_get(t,p); ePRIV_PROP_DEC_set(t,p)
			#define ePRIV_PROP_DEC_PUB(t, p, m) ePRIV_PROP_DEC_##m(t,p)
		
		/**
		* private property PRIVATE function-pointer DECLARATIONS for private struct
		* @param t Type
		* @param p Property name
		* @param m get/set/getset
		*/
			#define ePRIV_PROP_DEC_PRIV(t, p, m) ePROP_DEC(t,p); ePRIV_PROP_DEC_##m(t,p)
		
		/**
		* private property PUBLIC function DECLARATIONS
		* @param c Type
		* @param t Function return type
		* @param p Property name
		* @param m get/set/getset
		*/
			#define ePRIV_PROP_FUNC_DEC_get(c, t, p) t c##_get_##p(void * eOBJ);
			#define ePRIV_PROP_FUNC_DEC_set(c, t, p) void c##_set_##p(void * eOBJ, t v );
			#define ePRIV_PROP_FUNC_DEC_getset(c, t, p) ePRIV_PROP_FUNC_DEC_get(c, t, p) ePRIV_PROP_FUNC_DEC_set(c, t, p)
			#define ePRIV_PROP_FUNC_DEC(c, t, p, m) ePRIV_PROP_FUNC_DEC_##m(c, t, p)
		
		/**
		* private property function ALLOCATIONS for object function pointers in instantiate
		* @param c Type
		* @param p Property name
		* @param v Property value
		* @param m get/set/getset
		*/
			#define ePRIV_PROP_DEF_get(c, p) self->get_##p = &c##_get_##p
			#define ePRIV_PROP_DEF_set(c, p) self->set_##p = &c##_set_##p
			#define ePRIV_PROP_DEF_getset(c, p) ePRIV_PROP_DEF_get(c, p); ePRIV_PROP_DEF_set(c, p)
			#define ePRIV_PROP_DEF(c, p, v, m) self->p = v; ePRIV_PROP_DEF_##m(c, p)
		
		/**
		* private property PUBLIC get/set function DEFINITIONS
		* @param c Type
		* @param t Function return type
		* @param p Property name
		* @param m get/set/getset
		*/
			#define ePRIV_PROP_FUNC_DEF_get(c, t, p) t c##_get_##p(void * eOBJ){ eSELF(c); return self->p; }
			#define ePRIV_PROP_FUNC_DEF_set(c, t, p) void c##_set_##p(void * eOBJ, t v ){ eSELF(c); self->p = v; }
			#define ePRIV_PROP_FUNC_DEF_getset(c, t, p) ePRIV_PROP_FUNC_DEF_get(c, t, p) ePRIV_PROP_FUNC_DEF_set(c, t, p)
			#define ePRIV_PROP_FUNC_DEF(c, t, p, m) ePRIV_PROP_FUNC_DEF_##m(c, t, p)
		
		
		
		/**
		* Cast "self" back into the class type
		* @param c Struct type
		*/
			#define eSELF(c) c * self = (c*)eOBJ
		
		/**
		* upCast "self" to parent struct type. Parent struct type must be first struct member publically
		* Can only be called from scope where child object is instantiated
		* @param c Struct type
		* @param v Variable name to use
		*/
			#define ePARENT(c, v) struct c * v = (struct c *)self
		
		/**
		* Get the value of a protected variable 'p' within object 'o'
		* Prints to stderr if property is private
		* @param o Object
		* @param p Object property
		*/
			#define eGET(o, p) o->get_##p(o)
		
		/**
		* Set the value of a protected variable 'x' within object 'o'
		* Prints to stderr if property is private
		* @param var o Object
		* @param var p Object property
		* @param var v The new value
		*/
			#define eSET(o, p, v) o->set_##p(o, v)
		
		/**
		* Method call wrapper that passes object as first argument for use of eSELF()
		* @param o Object
		* @param m The method
		* @param ... Other args
		*/
			#define eCALL(o, m, ...) (*o->m)(o, __VA_ARGS__)
		
		/**
		* Method call wrapper that passes object as first argument for use of eSELF(), no arguments
		* @param o Object
		* @param m The method
		* @param ... Other args
		*/
			#define eCALLna(o,m) (*o->m)(o)
			
		/**
		* Free memory on heap for object
		* @param var o Object variable name
		*/
			#define eDESTROY(o) free(o); o = ((void*)0)
		
#endif //EOOPC_MAIN_H
