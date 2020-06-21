depend IntNode implements Node<int32> from "Stack.ifc";
depend IntStack implements Stack<int32> from "Stack.ifc";

test("myNodeTest")
{
	IntNode n0(3);
	IntNode n1(5, &n0);
	assert n1->get() == 5;
	assert n1->pop() == &n0;
	IntNode n2(3);
	n2.push(&n0);
	assert n2.pop() == &n0;
}

test("myStackTest")
{
	IntStack stack;
	stack.push(5);
	stack.push(3);
	stack.push(6);
	assert stack.get() == 6;
	assert stack.pop() == 6;
	stack.drop();
	assert stack.pop() == 5;
}
