module;

#include "stdafx.h"

export module ternary_search_tree_map;

import std;

using namespace std;

export template<typename TValue>
class TernarySearchTreeMap
{
	struct Node
	{
		byte key_part;
		unique_ptr<Node> left, right, middle;
		TValue value;
	};
	unique_ptr<Node> root;

	optional<Node&> search(const span<byte>& key, bool create = false)
	{
		if (root == nullptr)
		{
			if (create)
			{
				root = make_unique<Node>();
				root->key_part = key[0];
			}
			else
			{
				return nullopt;
			}
		}
		return {};
	}

public:
};