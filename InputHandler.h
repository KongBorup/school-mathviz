#pragma once

#include <string>
#include <sstream>
#include <cctype>
#include <vector>
#include "Matrix_NxN.h"
#include "Parser.h"

struct BadInputFormat : public std::exception {};
struct UnknownIdentifier : public std::exception {};

enum InputKind
{
	vect,  // V(...)
	func,  // F(...)
	point, // P(...)
};

class InputHandler
{
private:
	std::string inp;

	InputKind deduce_inp_kind(std::string str)
	{
		// Input length must be at least 4. 1 char for identifier,
		// 2 chars for parentheses, and at least 1 char for content.
		if (str.length() < 4 || !isalpha(str[0]) || str[1] != '(' || str[str.length() - 1] != ')')
		{
			throw BadInputFormat();
		}

		char ident = str[0];

		switch (ident)
		{
		case 'V':
			return InputKind::vect;
			break;

		case 'F':
			return InputKind::func;
			break;

		case 'P':
			return InputKind::point;
			break;

		default:
			throw UnknownIdentifier();
			break;
		}
	}

public:
	InputKind inp_kind;

	InputHandler(std::string _inp)
	{
		inp = _inp;
		inp_kind = this->deduce_inp_kind(inp);
	}

	Vector_N<2> evaluate_vec()
	{
		std::stringstream cont_ss;

		for (int i = 2; i < inp.length() - 1; i++)
		{
			cont_ss << inp[i];
		}

		std::string content = cont_ss.str();

		Parser p(content);
		return p.eval_expr_vec();
	}

	std::vector<Vector_N<2>> evaluate_func(double from, double to, double spacing)
	{
		std::vector<Vector_N<2>> data;

		for (double x = from; x <= to; x += spacing)
		{
			
			std::stringstream cont_ss;
			for (int i = 2; i < inp.length() - 1; i++)
			{
				if (inp[i] == 'x')
					cont_ss << '(' << (x < 0 ? "0" : "") << std::to_string(x) << ')';
				else
				{
					cont_ss << inp[i];
				}
			}
			std::string content = cont_ss.str();

			Parser p(content);
			double y = p.eval_expr_num();

			double varr[2] = { x, y };
			Vector_N<2> dp(varr);

			data.push_back(dp);
		}

		return data;
	}
};