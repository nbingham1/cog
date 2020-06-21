typename ValueType;

import Stack<ValueType> from "Stack.cog" {
	// This dependency resolution isn't necessary
	// because of the suggest in Stack.cog
	Node = Node<ValueType> from "Node.cog";
}

process main()
{
	// Do something
}
