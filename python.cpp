
#include "stdafx.h"
#include "fixes.h"
#include "addresses.h"
#include "dependencies/python-2.2/Python.h"
#include "temple_functions.h""

/*
	Define all type objects used by ToEE.
*/
// ObjHandle
	static GlobalStruct<PyTypeObject, 0x102CF3B8> pyObjHandleType;
	static getattrfunc pyObjHandleTypeGetAttr; // Original getattr of pyObjHandleType
	static setattrfunc pyObjHandleTypeSetAttr; // Original setattr of pyObjHandleType
	static GlobalStruct<PyMethodDef, 0x102CE9A8> pyObjHandleMethods;
	//PyCFunction a[]; // this causes errors during compile!
	

struct ObjectId {
	uint16_t subtype;
	GUID guid;
};

struct TemplePyObjHandle : public PyObject {
	ObjectId objId;
	uint64_t objHandle;
};







static PyObject * pyObjHandleType_Faction_Has(TemplePyObjHandle* obj, PyObject * pyTupleIn){
	int nFac;
	if (!PyArg_ParseTuple(pyTupleIn, "i", &nFac)) {
		return nullptr;
	};
	//auto dude = (TemplePyObjHandle*)obj;

	//return PyInt_FromLong(templeFuncs.Obj_Faction_Has(dude->objHandle, nFac));
	return PyInt_FromLong(templeFuncs.Obj_Faction_Has(obj->objHandle, nFac));
};

static PyMethodDef pyObjHandleMethods_New[] = {
	"faction_has", (PyCFunction)pyObjHandleType_Faction_Has, METH_VARARGS, "Check if NPC has faction. Doesn't work on PCs!",
	"faction_has2", (PyCFunction)pyObjHandleType_Faction_Has, METH_VARARGS, "Check if NPC has faction. Doesn't work on PCs!",
	0, 0, 0, 0
};



PyObject* __cdecl  pyObjHandleType_getAttrNew(TemplePyObjHandle *obj, char *name) {
	LOG(info) << "Tried getting property: " << name;
	if (!_strcmpi(name, "co8rocks")) {
		return PyString_FromString("IT SURE DOES!");
	}

	if (!_strcmpi(name, "ObjHandle")) {
		return  PyLong_FromLongLong(obj->objHandle); 
	}

	if (!_strcmpi(name, "factions")) {
		ObjHndl ObjHnd = obj->objHandle;
		int a[50] = {};
		int n = 0;

		for (int i = 0; i < 50; i ++){
			int fac = templeFuncs.Obj_Get_IdxField_32bit(ObjHnd, obj_f_npc_faction, i);
			if (fac == 0) break;
			a[i] = fac;
			n++;
		};

		auto outTup = PyTuple_New(n);
		for (int i = 0; i < n ; i++){
			PyTuple_SetItem(outTup, i, PyInt_FromLong(a[i]));
		};

		
		return  outTup; 
	}

	if (!_strcmpi(name, "faction_has")) {
		//ObjHndl ObjSubsInv = templeFuncs.Obj_Get_Substitute_Inventory(obj->objHandle);
		
		
		return Py_FindMethod(pyObjHandleMethods_New, obj, "faction_has");
		
	}


	if (!_strcmpi(name, "substitute_inventory")) {
		ObjHndl ObjSubsInv = templeFuncs.Obj_Get_Substitute_Inventory(obj->objHandle);
		return templeFuncs.PyObj_From_ObjHnd(ObjSubsInv);
	}



	if (!_strcmpi(name, "obj_get_field_64")) {
		
		return 0; 
	}

	return pyObjHandleTypeGetAttr(obj, name);
}




int __cdecl  pyObjHandleType_setAttrNew(TemplePyObjHandle *obj, char *name, TemplePyObjHandle *obj2) {
	LOG(info) << "Tried setting property: " << name;
	if (!strcmp(name, "co8rocks")) {
		return 0;
	}

	if (!strcmp(name, "substitute_inventory")) {


		if (obj2 != nullptr)  {
			if (obj->ob_type == obj2->ob_type){
				templeFuncs.Obj_Set_Field_ObjHnd(obj->objHandle, obj_f_npc_substitute_inventory, obj2->objHandle);
			}
			
		}
		
		return 0;
	}


	return pyObjHandleTypeSetAttr(obj, name, obj2);
}

class PythonExtensions : public TempleFix {
public:
	const char* name() override {
		return "Python Script Extensions";
	}
	void apply() override;
} pythonExtension;

void PythonExtensions::apply() {

	// Hook the getattr function of obj handles
	pyObjHandleTypeGetAttr = pyObjHandleType->tp_getattr;
	pyObjHandleType->tp_getattr = (getattrfunc) pyObjHandleType_getAttrNew;

	pyObjHandleTypeSetAttr = pyObjHandleType->tp_setattr;
	pyObjHandleType->tp_setattr = (setattrfunc) pyObjHandleType_setAttrNew;


	//a[0] = pyObjHandleMethods->ml_meth;
	
}
