#include "Function.h"
#include "Stack.h"
#include "State.h"




InstructionValue::InstructionValue():
_parentInsVal(nullptr), _insSet(nullptr),
_hasFor(false), _breaked(false)
{

}


void InstructionValue::clearInsSet()
{
	if (_insSet)  {
		_insSet->clearInstructions();
	}
}

Function::Function()
{
}


Function::~Function()
{
}




Closure* Function::generateClosure(State* s)
{
	Closure* cl = new Closure(s);
	cl->setPrototype(this);
	return cl;
}





Closure::Closure(State* s)
	:_state(s), _actParamNum(0),
	_actRetNum(0), _needRetNum(0),
	_prototype(nullptr),
	_parentClosure(nullptr),
	_upTables(nullptr)
{

}

void Closure::initClosure()
{
	clearClosure();
	_nest_tables.push_back(new Table());
}

void Closure::clearClosure()
{
	_nest_tables.clear();
}

void Closure::balanceStack()
{
	if (_needRetNum != -1)  {                  //为-1时，表示无法计算需要的返回值，例如在f1(f2())
		int n = _actRetNum - _needRetNum;      //真实返回值个数-需要返回值个数
		if (n > 0)  {                           //f() + 2,如果f()真实返回了2个，但是这里只需要一个，故栈要弹出一个
			while (n > 0)  {
				_state->getStack()->popValue();
				n--;
			}
		}
		else  {
			while (n < 0)  {                    //f() + 2, 如果f()真实返回0个，但是这里需要一个，故要压入栈一个（貌似不压也行，因为后面赋值时会平衡）
				_state->getStack()->Push(new Nil());
				n++;
			}
		}
	}
	
	_actRetNum = 0;
	_needRetNum = 0;
}

void Closure::addBlockTable()
{
	_nest_tables.push_back(new Table());
}

void Closure::removeBlockTable()
{
	Table* top = getTopTable();
	if (top)  {
		delete top;
	}
	_nest_tables.pop_back();
}

Closure* Closure::clone()
{
	Closure* cl = new Closure(_state);
	cl->_needRetNum = _needRetNum;
	cl->_actParamNum = _actRetNum;
	cl->_actRetNum = _actRetNum;
	cl->_parentClosure = _parentClosure;
	cl->_prototype = _prototype;
	if (_upTables)  {
		cl->_upTables = _upTables->clone();
	}
	return cl;
}

Table* Closure::getTopTable()
{
	return getLevelTable(_nest_tables.size() - 1);
}

Table* Closure::getLevelTable(unsigned int i)
{
	if (_nest_tables.size() == 0  || i >= _nest_tables.size())  {
		return nullptr;
	}
	return _nest_tables[i];
}

void Closure::setParentClosure(Closure* c)
{
	_parentClosure = c;
	if (c)  {
		Table* topTab = c->getTopTable();       //只需拷贝最外层的局部变量,for循环里面的局部变量，子函数是不可见的
		if (topTab)  {
			_upTables = topTab->clone();
		}
	}
}


int Closure::findInNestTables(Value* key, Value** val)
{
	int num = _nest_tables.size();
	Value* temp = nullptr;
	for (int i = num - 1; i >= 0; i--)  {
		temp = _nest_tables[i]->GetValue(key);
		if (temp)  {
			if (val) *val = temp;
			return i;
		}
	}
	return -1;
}

int Closure::findUpTables(Value* key, Value** val, Table** tab)
{
	Closure* cl = this;
	while (cl)  {
		int level = cl->findInNestTables(key, val);
		if (level != -1)  {  //在当前闭包中找到
			if (tab) *tab = cl->getLevelTable(level);
			return 1;
		}
		cl = cl->_parentClosure;
	}

	if (_upTables)  {      //在其上值中找
		Value* temp = _upTables->GetValue(key);
		if (temp) { 
			if (val) *val = temp;
			if (tab) *tab = _upTables;
			return 2;
		}
	}

	Table* table = getState()->getGlobalTable();  //已到了最顶层
	Value* temp = table->GetValue(key);
	if (temp)  {
		if (val) *val = temp;
		if (tab) *tab = table;
		return 0;
	}
	else  {
		return -1;
	}
	return -1;
}