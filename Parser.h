#pragma once

#define _USE_MATH_DEFINES

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cctype>
#include <exception>
#include <algorithm>
#include <math.h>
#include "Matrix_NxN.h"

// Exception definitions
struct InvalidParentheses : public std::exception {};
struct MisplacedOperator : public std::exception {};
struct UnsuccesfulCalculation : public std::exception {};
struct InvalidFunction : public std::exception {};
struct InvalidMatrixOrVector : public std::exception {};
struct MustNotMultiplyVectors : public std::exception {};

enum TokenKind {
	p_start,	// (
	p_end,		// )
	v_start,	// [
	v_end,		// ]
	v_sep,		// ,
	add_op,		// +
	sub_op,		// -
	mul_op,		// *
	div_op,		// /
	pow_op,		// ^
	num,		// Number of any length
	function,	// sqrt, cos, sin etc.
	variable,	// x
	vec,		// A fully treated vector [x,y]. x and y are guaranteed to be single numbers.
	mat,		// A fully treated matrix [[x1, y1],[x2,y2]]. Same rule as above.
	unknown,	// Everything else
	end,		// Terminate expression
};

const std::vector<std::vector<TokenKind>> OP_PRECEDENCE = {
	{TokenKind::function}, // In a case of e.g. cos(x)^y, cos(x) shall be evaluated first. Not (x)^y.
	{TokenKind::pow_op},
	{TokenKind::mul_op, TokenKind::div_op},
	{TokenKind::add_op, TokenKind::sub_op},
};

struct Token {
public:
	std::string value;
	TokenKind type;

	Token(std::string val, TokenKind typ)
	{
		value = val;
		type = typ;
	}

	Token() {};
};

Token make_token(std::string val, TokenKind typ)
{
	Token tok(val, typ);
	return tok;
}

std::string stringify(char c)
{
	std::string str(1, c);
	return str;
}

// We need to convert strings to integers if we wish to use them in switch statements.
constexpr unsigned int str_to_int(const char* str, int h = 0)
{
	return !str[h] ? 5381 : (str_to_int(str, h + 1) * 33) ^ str[h];
}

class Tokenizer
{
	friend class Parser;

private:
	std::string inp;
	size_t i = 0;

	// Returns next character and advances the index
	char next()
	{
		return i < inp.length() && i >= 0 ? inp[i++] : NULL;
	}

	void step_back()
	{
		i--;
	}

	Token next_token()
	{
		char c = this->next();

		// We assume that any alphabetic characters will start a function such as cos, sin, sqrt etc.
		// x is exempt from this rule because it is reserved for variables.
		if (isalpha(c) && c != 'x')
		{
			goto parse_function;
		}

		switch (c)
		{
		case NULL:
			return make_token("", TokenKind::end);
			break;

		// Ignore all whitespace characters
		case ' ': case '\t': case '\r':
			return this->next_token();
			break;

		// Any number
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			goto parse_number;
			break;

		// Single-character tokens
		case '+': return make_token(stringify(c), TokenKind::add_op); break;
		case '-': return make_token(stringify(c), TokenKind::sub_op); break;
		case '*': return make_token(stringify(c), TokenKind::mul_op); break;
		case '/': return make_token(stringify(c), TokenKind::div_op); break;
		case '^': return make_token(stringify(c), TokenKind::pow_op); break;

		case '(': return make_token(stringify(c), TokenKind::p_start); break;
		case ')': return make_token(stringify(c), TokenKind::p_end); break;
		case '[': return make_token(stringify(c), TokenKind::v_start); break;
		case ']': return make_token(stringify(c), TokenKind::v_end); break;
		case ',': return make_token(stringify(c), TokenKind::v_sep); break;

		case 'x': return make_token(stringify(c), TokenKind::variable); break;

		// Unrecognized token
		default:
			return make_token(stringify(c), TokenKind::unknown);
			break;
		}

	parse_number:
		{
			std::stringstream num;
			num << c;

			while (char nc = this->next())
			{
				switch (nc)
				{
				// Any number OR period (decimal separator)
				case '0': case '1': case '2': case '3': case '4':
				case '5': case '6': case '7': case '8': case '9':
				case '.':
					num << nc;
					break;

				default:
					this->step_back();
					goto after_loop_num;
					break;
				}
			}

			after_loop_num:
				return make_token(num.str(), TokenKind::num);
		}
		
	parse_function:
		{
			std::stringstream func;
			func << c;

			while (char fc = this->next())
			{
				if (isalpha(fc))
				{
					func << fc;
				}
				else
				{
					this->step_back();

					std::string func_str = func.str();

					// Replace constants such as pi and e with their number value.
					switch (str_to_int(func_str.c_str()))
					{
					case str_to_int("pi"):
						return make_token(std::to_string(M_PI), TokenKind::num);
						break;

					case str_to_int("e"):
						return make_token(std::to_string(M_E), TokenKind::num);
						break;

					// If the letters don't correspond to a constant, then it is a function such as cos, sin, log etc.
					default:
						return make_token(func_str, TokenKind::function);
						break;
					}
					
				}
			}
		}
	}

	// Parses the input string into tokens.
	std::vector<Token> tokenize()
	{
		std::vector<Token> tokens;

		Token tok = this->next_token();

		while (tok.type != TokenKind::end) {
			// This should probably throw an error, but we can just ignore unknown symbols.
			if (tok.type != TokenKind::unknown)
			{
				tokens.push_back(tok);
			}

			tok = this->next_token();
		}

		return tokens;
	}

public:
	Tokenizer() {}

	Tokenizer(std::string input)
	{
		// We add a space to the end so that it will never terminate too early.
		// This makes sure that `e` is correctly parsed when at the end of `pi + e` for example.
		inp = input + " ";
	}
};

class InfixTree
{
public:
	Token op;
	InfixTree* arg1;
	InfixTree* arg2;

	InfixTree(Token _op)
	{
		op = _op;
		arg1 = NULL;
		arg2 = NULL;
	}

	void set_arg1(InfixTree* _arg1)
	{
		arg1 = _arg1;
	}

	void set_arg2(InfixTree* _arg2)
	{
		arg2 = _arg2;
	}

	void print()
	{
		std::cout << "Op: " << op.value << std::endl;
		std::cout << "Arg 1: " << (arg1 == NULL ? "NULL" : arg1->op.value) << std::endl;
		std::cout << "Arg 2: " << (arg2 == NULL ? "NULL" : arg2->op.value) << std::endl;
	}
};

bool is_in_vec(std::vector<TokenKind> v, TokenKind val)
{
	return std::find(v.begin(), v.end(), val) != v.end();
}

class Evaluator
{
private:
	std::vector<Token> tokens;
	size_t i = 0;
	size_t op_prec_row = 3;

	Token next_token()
	{
		return i < tokens.size() && i >= 0 ? tokens[i++] : make_token("", TokenKind::end);
	}

	Token previous_token()
	{
		// Subtract two because next_token() advances AFTER retrieving value at index.
		return tokens[i - 2];
	}

	void step_back()
	{
		i--;
	}

	void reset_walker()
	{
		i = 0;
	}

	void move_walker_to(size_t index)
	{
		i = index;
	}

	void advance_operator_precedence()
	{
		op_prec_row++;
	}

    // In the case of the token list: {"5.6", "+", "7.5"}, this function saves the left and right argument to the node tree
	void add_2_args_to_node(InfixTree* node)
	{
		if (i > 1)
		{
			Token prv = this->previous_token();
			InfixTree* arg_left = prv.type == TokenKind::num || prv.type == TokenKind::vec || prv.type == TokenKind::mat
				? new InfixTree(prv) : NULL;
			node->set_arg1(arg_left);
		}
		
		Token nxt = this->next_token();
		InfixTree* arg_right = nxt.type == TokenKind::p_start ?
			new InfixTree(make_token(std::to_string(this->eval_p_group()), TokenKind::num)) :
			new InfixTree(nxt);

		this->step_back();

		if (arg_right->op.type == TokenKind::end) throw MisplacedOperator();
		
		node->set_arg2(arg_right);
	}

	// Assumes that the string is a correct number and doesn't contain any invalid characters!
	double str_to_num(std::string str)
	{
		std::stringstream num_str(str);
		double num;
		num_str >> num;

		return num;
	}

	Vector_N<2> str_to_vec(std::string str)
	{
		double x, y;
		sscanf_s(str.c_str(), "[%lf,%lf]", &x, &y);
		double varr[2] = { x, y };
		Vector_N<2> res_vec(varr);

		return res_vec;
	}

	std::string vec_to_str(Vector_N<2> vec)
	{
		std::stringstream ss;
		ss << '[' << vec.get_x() << ',' << vec.get_y() << ']';
		return ss.str();
	}

	Matrix_NxN<2, 2> str_to_mat(std::string str)
	{
		double x1, y1, x2, y2;
		sscanf_s(str.c_str(), "[[%lf,%lf],[%lf,%lf]]", &x1, &y1, &x2, &y2);

		double marr[2][2] = {
			{ x1, x2 },
			{ y1, y2 }
		};
		Matrix_NxN<2, 2> res_mat(marr);

		return res_mat;
	}

	std::string mat_to_str(Matrix_NxN<2, 2> mat)
	{
		std::stringstream ss;
		ss << "[[" << mat.mat[0][0] << ',' << mat.mat[1][0]
			<< "],[" <<  mat.mat[0][1] << ',' << mat.mat[1][1] << "]]";
		return ss.str();
	}

    // Evaluates a full parenthesis group and returns the result of it.
	double eval_p_group()
	{
		int depth = 1;
		std::vector<Token> p_group;

		for (int j = i; i < this->tokens.size(); j++)
		{
			Token g_tok = this->next_token();

			if (g_tok.type == TokenKind::p_start) depth++;
			else if (g_tok.type == TokenKind::p_end) depth--;

			if (depth == 0)
			{
				break;
			}

			p_group.push_back(g_tok);
		}

		Evaluator evaluator(p_group);
		double p_eval = evaluator.eval();

		return p_eval;
	}

    // Receives a lambda function that treats the node's arguments
	template<typename F>
	double calculate_result(InfixTree* node, F lambda)
	{
		double num1 = node->arg1 == NULL ? 0.0 : this->str_to_num(node->arg1->op.value);
		double num2 = node->arg2 == NULL ? 0.0 : this->str_to_num(node->arg2->op.value);

		return lambda(num1, num2);
	}
	
	void replace_tokens_with_result(double res, size_t s, size_t e)
	{
		tokens.erase(tokens.begin() + s, tokens.begin() + e);
		tokens.insert(tokens.begin() + s, make_token(std::to_string(res), TokenKind::num));
	}

	void replace_tokens_with_token(Token tok, size_t s, size_t e)
	{
		tokens.erase(tokens.begin() + s, tokens.begin() + e);
		tokens.insert(tokens.begin() + s, tok);
	}

	double eval_vec_entry()
	{
		std::vector<Token> vec_entry;

		for (int j = i; i < this->tokens.size(); j++)
		{
			Token g_tok = this->next_token();

			if (g_tok.type == TokenKind::v_sep || g_tok.type == TokenKind::v_end)
			{
				break;
			}

			vec_entry.push_back(g_tok);
		}

		Evaluator evaluator(vec_entry);
		double p_eval = evaluator.eval();

		return p_eval;
	}

public:
	Evaluator(std::vector<Token> toks)
	{
		tokens = toks;
	}

    // The main evaluator function. Converts a mathematical expression to a single number.
	double eval()
	{
		for (std::vector<TokenKind> cur_ops : OP_PRECEDENCE)
		{
			Token tok = this->next_token();

			while (tok.type != TokenKind::end)
			{
				InfixTree* node = new InfixTree(tok);

				switch (tok.type)
				{
				case TokenKind::p_start:
				{
					size_t s = i - 1;
					double res = this->eval_p_group();
					size_t e = i;

					this->replace_tokens_with_result(res, s, e);
					this->move_walker_to(s);
				}
				break;

				case TokenKind::mul_op:
					if (is_in_vec(cur_ops, TokenKind::mul_op))
					{
						size_t s = i - 2;
						this->add_2_args_to_node(node);
						size_t e = i;

						double prod = this->calculate_result(node,
							[](double num1, double num2) { return num1 * num2; });
						this->replace_tokens_with_result(prod, s, e + 1);
						this->move_walker_to(s);
					}
					break;

				case TokenKind::div_op:
					if (is_in_vec(cur_ops, TokenKind::mul_op))
					{
						size_t s = i - 2;
						this->add_2_args_to_node(node);
						size_t e = i;

						double prod = this->calculate_result(node,
							[](double num1, double num2) { return num1 / num2; });
						this->replace_tokens_with_result(prod, s, e + 1);
						this->move_walker_to(s);
					}
					break;

				case TokenKind::add_op:
					if (is_in_vec(cur_ops, TokenKind::add_op))
					{
						size_t s = i - 2;
						this->add_2_args_to_node(node);
						size_t e = i;

						double sum = this->calculate_result(node,
							[](double num1, double num2) { return num1 + num2; });
						this->replace_tokens_with_result(sum, s, e + 1);
						this->move_walker_to(s);
					}
					break;

				case TokenKind::sub_op:
					if (is_in_vec(cur_ops, TokenKind::sub_op))
					{
						size_t s = i - 2;
						this->add_2_args_to_node(node);
						size_t e = i;

						double sum = this->calculate_result(node,
							[](double num1, double num2) { return num1 - num2; });
						this->replace_tokens_with_result(sum, s, e + 1);
						this->move_walker_to(s);
					}
					break;

				case TokenKind::pow_op:
					if (is_in_vec(cur_ops, TokenKind::pow_op))
					{
						size_t s = i - 2;
						this->add_2_args_to_node(node);
						size_t e = i;

						double sum = this->calculate_result(node,
							[](double num1, double num2) { return pow(num1, num2); });
						this->replace_tokens_with_result(sum, s, e + 1);
						this->move_walker_to(s);
					}
					break;

				case TokenKind::function:
					if (is_in_vec(cur_ops, TokenKind::function))
					{
						size_t s = i - 1;
						this->add_2_args_to_node(node);
						size_t e = i;

						double func_res;

						switch (str_to_int(tok.value.c_str()))
						{
						case str_to_int("cos"):
							func_res = this->calculate_result(node,
								[](double num1, double num2) { return cos(num2); });
							break;

						case str_to_int("sin"):
							func_res = this->calculate_result(node,
								[](double num1, double num2) { return sin(num2); });
							break;

						case str_to_int("tan"):
							func_res = this->calculate_result(node,
								[](double num1, double num2) { return tan(num2); });
							break;

						case str_to_int("sqrt"):
							func_res = this->calculate_result(node,
								[](double num1, double num2) { return sqrt(num2); });
							break;

						// log as defined by math.h is the natural logarithm. We will stick to log being log10.
						case str_to_int("ln"):
							func_res = this->calculate_result(node,
								[](double num1, double num2) { return log(num2); });
							break;

						case str_to_int("log"):
							func_res = this->calculate_result(node,
								[](double num1, double num2) { return log10(num2); });
							break;

						default:
							throw InvalidFunction();
							break;
						}
						 
						this->replace_tokens_with_result(func_res, s, e + 1);
						this->move_walker_to(s);
					}
					break;

				default:
					break;
				}

				tok = this->next_token();

				// We created the InfixTree* node pointer using the `new` keyword, making sure it was
				// stored on the heap. Thus we also need to delete it ourselves to avoid leaking memory:
				delete node->arg1;
				delete node->arg2;
				delete node;
			}

			this->reset_walker();
		}

		// If everything went according to plan, we should only have 1 single token of type num left.
		Token result = this->next_token();
		if (tokens.size() != 1 || result.type != TokenKind::num)
		{
			throw UnsuccesfulCalculation();
		}

		return str_to_num(result.value);
	}

    // Evaluates a math expression that results in a vector
	Vector_N<2> eval_vec()
	{
		Token tok = this->next_token();

		// Go through everything and evaluate the contents of vectors and matrices.
		while (tok.type != TokenKind::end)
		{
			switch (tok.type)
			{
			case TokenKind::v_start:
			case TokenKind::v_sep:
			{
				Token nxt = this->next_token();
				if (nxt.type == TokenKind::v_start)
				{
					tok = nxt;
				}
				else
				{
					this->step_back();
				}

				size_t s = i - 1;
				double res = this->eval_vec_entry();
				size_t e = i;

				this->replace_tokens_with_result(res, s + 1, e - 1);
				this->move_walker_to(s + 1);
			}
				break;

			default:
				break;
			}

			tok = this->next_token();
		}

		this->reset_walker();
		tok = this->next_token();

		// Go through again and convert vectors that are now split into multiple tokens into a single token.
		while (tok.type != TokenKind::end)
		{
			switch (tok.type)
			{
			case TokenKind::v_start:
			{
				// Jump to the next token. If it is a vec start too, we simply start evaluating from there.
				Token nxt = this->next_token();
				if (nxt.type == TokenKind::v_start)
				{
					tok = nxt;
				}
				else
				{
					this->step_back();
				}

				size_t s = i - 1;

				std::stringstream vec;
				vec << tok.value;

				Token tmp_tok = this->next_token();

				while (tmp_tok.type != TokenKind::end)
				{
					vec << tmp_tok.value;

					if (tmp_tok.type == TokenKind::v_end)
					{
						break;
					}

					tmp_tok = this->next_token();
				}

				Token vec_tok = make_token(vec.str(), TokenKind::vec);

				size_t e = i;

				this->replace_tokens_with_token(vec_tok, s, e);
				this->move_walker_to(s + 1);
			}
			break;

			default:
				break;
			}

			tok = this->next_token();
		}

		this->reset_walker();
		tok = this->next_token();

		// Go through again and construct potential matrices
		while (tok.type != TokenKind::end)
		{
			switch (tok.type)
			{
			case TokenKind::v_start:
			{
				size_t s = i - 1;

				std::stringstream mat;
				mat << tok.value;

				Token tmp_tok = this->next_token();

				while (tmp_tok.type != TokenKind::end)
				{
					mat << tmp_tok.value;

					if (tmp_tok.type == TokenKind::v_end)
					{
						break;
					}

					tmp_tok = this->next_token();
				}

				Token mat_tok = make_token(mat.str(), TokenKind::mat);

				size_t e = i;

				this->replace_tokens_with_token(mat_tok, s, e);
				this->move_walker_to(s + 1);
			}
			break;

			default:
				break;
			}

			tok = this->next_token();
		}

		// Finally, do all the arithmetic
		this->reset_walker();
		tok = this->next_token();

		for (std::vector<TokenKind> cur_ops : OP_PRECEDENCE)
		{
			Token tok = this->next_token();

			while (tok.type != TokenKind::end)
			{
				InfixTree* node = new InfixTree(tok);

				switch (tok.type)
				{
				case TokenKind::add_op:
					if (is_in_vec(cur_ops, TokenKind::add_op))
					{
						size_t s = i - 2;
						this->add_2_args_to_node(node);
						size_t e = i;

						if (node->arg1->op.type == TokenKind::vec)
						{
							Vector_N<2> v1 = this->str_to_vec(node->arg1->op.value);
							Vector_N<2> v2 = this->str_to_vec(node->arg2->op.value);

							Vector_N<2> sum = v1.add(v2).to_vec();

							Token res_tok = make_token(this->vec_to_str(sum), TokenKind::vec);
							this->replace_tokens_with_token(res_tok, s, e + 1);
						}

						if (node->arg1->op.type == TokenKind::mat)
						{
							Matrix_NxN<2, 2> m1 = this->str_to_mat(node->arg1->op.value);
							Matrix_NxN<2, 2> m2 = this->str_to_mat(node->arg2->op.value);

							Matrix_NxN<2, 2> sum = m1.add(m2);

							Token res_tok = make_token(this->mat_to_str(sum), TokenKind::mat);
							this->replace_tokens_with_token(res_tok, s, e + 1);
						}
						
						this->move_walker_to(s);
					}
					break;

				case TokenKind::sub_op:
					if (is_in_vec(cur_ops, TokenKind::sub_op))
					{
						size_t s = i - 2;
						this->add_2_args_to_node(node);
						size_t e = i;

						if (node->arg1->op.type == TokenKind::vec)
						{
							Vector_N<2> v1 = this->str_to_vec(node->arg1->op.value);
							Vector_N<2> v2 = this->str_to_vec(node->arg2->op.value);

							Vector_N<2> sum = v1.subtract(v2).to_vec();

							Token res_tok = make_token(this->vec_to_str(sum), TokenKind::vec);
							this->replace_tokens_with_token(res_tok, s, e + 1);
						}

						if (node->arg1->op.type == TokenKind::mat)
						{
							Matrix_NxN<2, 2> m1 = this->str_to_mat(node->arg1->op.value);
							Matrix_NxN<2, 2> m2 = this->str_to_mat(node->arg2->op.value);

							Matrix_NxN<2, 2> sum = m1.subtract(m2);

							Token res_tok = make_token(this->mat_to_str(sum), TokenKind::mat);
							this->replace_tokens_with_token(res_tok, s, e + 1);
						}
						
						this->move_walker_to(s);
					}
					break;

				case TokenKind::mul_op:
					if (is_in_vec(cur_ops, TokenKind::sub_op))
					{
						size_t s = i - 2;
						this->add_2_args_to_node(node);
						size_t e = i;

						if (node->arg2->op.type == TokenKind::vec)
						{
							if (node->arg1->op.type == TokenKind::vec) throw MustNotMultiplyVectors();

							if (node->arg1->op.type == TokenKind::num)
							{
								double n = this->str_to_num(node->arg1->op.value);
								Vector_N<2> v = this->str_to_vec(node->arg2->op.value);

								Vector_N<2> prod = v.mult(n).to_vec();

								Token res_tok = make_token(this->vec_to_str(prod), TokenKind::vec);
								this->replace_tokens_with_token(res_tok, s, e + 1);
							}
							else if (node->arg1->op.type == TokenKind::mat)
							{
								Matrix_NxN<2, 2> m = this->str_to_mat(node->arg1->op.value);
								Vector_N<2> v = this->str_to_vec(node->arg2->op.value);

								Vector_N<2> prod = m.mult(v).to_vec();

								Token res_tok = make_token(this->vec_to_str(prod), TokenKind::vec);
								this->replace_tokens_with_token(res_tok, s, e + 1);
							}
						}

						if (node->arg1->op.type == TokenKind::mat && node->arg2->op.type == TokenKind::mat)
						{
							Matrix_NxN<2, 2> m1 = this->str_to_mat(node->arg1->op.value);
							Matrix_NxN<2, 2> m2 = this->str_to_mat(node->arg2->op.value);

							m1.print();
							m2.print();

							Matrix_NxN<2, 2> prod = m1.mult(m2);

							Token res_tok = make_token(this->mat_to_str(prod), TokenKind::mat);
							this->replace_tokens_with_token(res_tok, s, e + 1);
						}

						if (node->arg1->op.type == TokenKind::num && node->arg2->op.type == TokenKind::mat)
						{
							double n = this->str_to_num(node->arg1->op.value);
							Matrix_NxN<2, 2> m = this->str_to_mat(node->arg2->op.value);

							Matrix_NxN<2, 2> prod = m.mult(n);

							Token res_tok = make_token(this->mat_to_str(prod), TokenKind::mat);
							this->replace_tokens_with_token(res_tok, s, e + 1);
						}

						this->move_walker_to(s);
					}
					break;

				default:
					break;
				}

				tok = this->next_token();

				// We created the InfixTree* node pointer using the `new` keyword, making sure it was
				// stored on the heap. Thus we also need to delete it ourselves to avoid leaking memory:
				delete node->arg1;
				delete node->arg2;
				delete node;
			}

			this->reset_walker();
		}
		
		this->reset_walker();
		// If everything went according to plan, we should only have 1 single token of type vec left.
		Token result = this->next_token();
		if (tokens.size() != 1 || result.type != TokenKind::vec)
		{
			throw UnsuccesfulCalculation();
		}

		Vector_N<2> res_vec = this->str_to_vec(result.value);

		return res_vec;
	}

	void print_toks()
	{
		for (Token token : tokens)
		{
			std::cout << "<" << token.value << "> ";
		}
		std::cout << std::endl;
	}
};

class Parser
{
private:
	std::string inp;
	Tokenizer tokenizer;
	std::vector<Token> tokens;
	size_t i = 0;

	Token next_token()
	{
		return i < tokens.size() && i >= 0 ? tokens[i++] : make_token("", TokenKind::end);
	}

	Token previous_token()
	{
		// Subtract two because next_token() advances AFTER retrieving value at index.
		return tokens[i - 2];
	}

	void step_back()
	{
		i--;
	}

	void check_parentheses()
	{
		int p_depth = 0;

		for (Token token : tokens)
		{
			if (token.type == TokenKind::p_start) p_depth++;
			else if (token.type == TokenKind::p_end) p_depth--;
		}

		if (p_depth != 0) throw InvalidParentheses();
	}

	void check_v_parentheses()
	{
		int v_depth = 0;

		for (Token token : tokens)
		{
			if (token.type == TokenKind::v_start) v_depth++;
			else if (token.type == TokenKind::v_end) v_depth--;

			if (v_depth < 0 || v_depth > 2) throw InvalidMatrixOrVector();
		}

		if (v_depth != 0) throw InvalidMatrixOrVector();
	}

public:
	Parser(std::string input)
	{
		inp = input;
		tokenizer = *new Tokenizer(input);
	}

	// Executes the mathematical expression (input string).
	double eval_expr_num()
	{
		tokens = this->tokenizer.tokenize();

		this->check_parentheses();

		Evaluator evaluator(tokens);
		double res = evaluator.eval();

		return res;
	}

	Vector_N<2> eval_expr_vec()
	{
		tokens = this->tokenizer.tokenize();

		this->check_parentheses();
		this->check_v_parentheses();

		Evaluator evaluator(tokens);
		Vector_N<2> res = evaluator.eval_vec();

		return res;
	}

	void print()
	{
		std::cout << inp << std::endl;
	}
};
