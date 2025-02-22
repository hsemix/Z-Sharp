﻿
#include <iostream>
#include <fstream>
#include <string>
#define DEVELOPER_MESSAGES false
#define EXAMPLE_PROJECT false
#define NAMEVERSION "ZSharp v2.0.2"

#if defined(__unix__)
#define UNIX true
#define WINDOWS false
#elif defined(_MSC_VER)
#define UNIX false
#define WINDOWS true
#endif

#include <regex>
#include <limits>
#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <boost/any.hpp>
#include <unordered_map>
#include <stdio.h>
#include <codecvt>

#if UNIX
#include <unistd.h>
#include <filesystem>
#elif WINDOWS
#include <windows.h>
#endif

#include "eval.h"
#include "strops.h"
#include "builtin.h"
#include "main.h"
#include "anyops.h"

#include "ZS.h"

using namespace std;
using namespace boost;

unordered_map<string, boost::any> globalVariableValues;
unordered_map<string, vector<vector<string>>> functionValues;

boost::any GetVariableValue(const string& varName, const unordered_map<string, boost::any>& variableValues)
{
	string classSubComponent;
	string baseName = trim(varName);

	if (count(varName, '.') > 0)
	{
		classSubComponent = trim(varName.substr(indexInStr(varName, '.') + 1, -1));
		baseName = trim(split(varName, '.')[0]);
	}

	boost::any outputValue = nullType;

	auto iA = variableValues.find(baseName);
	auto iB = globalVariableValues.find(baseName);
	if (iA != variableValues.end())
		outputValue = iA->second;
	else if (iB != globalVariableValues.end())
		outputValue = iB->second;
	else
		outputValue = varName;

	if (count(varName, '.') > 0 && !outputValue.empty())
		return GetClassSubComponent(outputValue, classSubComponent);
	else
		return outputValue;
}

// Check if there is a variable with the specified name
bool IsVar(const string& varName, const unordered_map<string, boost::any>& variableValues)
{
	if (variableValues.find(split(varName, '.')[0]) != variableValues.end() && split(varName, '.')[0] != "ZS")
		return true;
	else
		return false;
}

// Return a vector of values that correspond to a vector of input variable names
vector<boost::any> VarValues(const vector<string>& varNames, unordered_map<string, boost::any>& variableValues)
{
	vector<boost::any> realValues;

	for (int varIndex = 0; varIndex < varNames.size(); varIndex++)
	{
		string varName = trim(varNames.at(varIndex));

		//realValues.push_back(EvalExpression(varName, variableValues));
		auto iA = variableValues.find(varName);
		if (iA != variableValues.end())
		{
			realValues.push_back(iA->second);
		}
		else
		{
			auto iB = globalVariableValues.find(varName);
			if (iB != globalVariableValues.end())
				realValues.push_back(iB->second);
			else
				realValues.push_back(EvalExpression(varName, variableValues));
		}
	}

	return realValues;
}

bool IsFunction(const string& funcName)
{
	if (functionValues.find(funcName) != functionValues.end())
		return true;
	else
		return false;
}
bool IsZSFunction(const string& funcName)
{
	if (funcName[0] == 'Z' && funcName[1] == 'S' && funcName[2] == '.')
		return true;
	else
		return false;
}

boost::any EvalExpression(const string& ex, unordered_map<string, boost::any>& variableValues)
{
	string expression = trim(ex);
	bool inQuotes = false;

#if DEVELOPER_MESSAGES == true
	//InterpreterLog("	old expression: |" + expression + "|");
#endif

	bool isFunc = IsFunction(split(expression, '(')[0]);
	bool isZS = split(expression, '.')[0] == "ZS";
	// If no operations are applied, then return self
	if ((countOutsideParenthesis(expression, '+') == 0 && countOutsideParenthesis(expression, '-') == 0 && countOutsideParenthesis(expression, '*') == 0 && countOutsideParenthesis(expression, '/') == 0 && countOutsideParenthesis(expression, '^') == 0) || split(expression, '.')[0] == "ZS")
	{
		bool isFunc = IsFunction(split(expression, '(')[0]);
		if (isFunc && !inQuotes)
		{
			//cout << split(expression, '(')[0] << endl;
			string argContents = "";
			int y = indexInStr(expression, '(') + 1;
			while (y < expression.size() && expression[y] != ')')
			{
				argContents += expression[y];

				y++;
			}
			return ExecuteFunction(split(expression, '(')[0], VarValues(split(argContents, ','), variableValues));
		}
		else if (split(expression, '.')[0] == "ZS" && !inQuotes)
		{
			string argContents = "";
			int y = indexInStr(expression, '(') + 1;
			while (y < expression.size() && expression[y] != ')')
			{
				argContents += expression[y];

				y++;
			}
			return ZSFunction(split(expression, '(')[0], VarValues(split(argContents, ','), variableValues));
		}
		else
			return GetVariableValue(expression, variableValues);
	}

	string newExpression = "";
	inQuotes = false;

	for (int i = 0; i < expression.size(); i++)
	{
		if (expression[i] == '\"' && !isEscaped(newExpression, i))
			inQuotes = !inQuotes;

		if (isalpha(expression[i]))
		{
			string name = "";

			while (i < expression.size() && (isalpha(expression[i]) || expression[i] == '.'))
			{
				name += expression[i];
				i++;
			}

			//string varVal = GetVariableValue(name, variables, variableValues);
			bool isFunc = IsFunction(name);
			if (isFunc && !inQuotes)
			{
				string argContents = "";
				i++;
				while (i < expression.size() && expression[i] != ')')
				{
					argContents += expression[i];

					i++;
				}
				string returnVal = AnyAsString(ExecuteFunction(name, VarValues(split(argContents, ','), variableValues)));
				newExpression += returnVal;
			}
			else if (split(name, '.')[0] == "ZS" && !inQuotes)
			{
				string argContents = "";
				int y = indexInStr(expression, '(') + 1;
				while (y < expression.size() && expression[y] != ')')
				{
					argContents += expression[y];

					y++;
				}
				//cout << split(expression, '(')[0] << " " << argContents << endl;
				string returnVal = AnyAsString(ZSFunction(split(name, '(')[0], VarValues(split(argContents, ','), variableValues)));
				newExpression += returnVal;
			}
			else
			{
				if (inQuotes)
					newExpression += name;
				else
					newExpression += AnyAsString(GetVariableValue(name, variableValues));
			}

			i--;
		}
		else
		{
			newExpression += expression[i];
		}
	}
#if DEVELOPER_MESSAGES == true
	//InterpreterLog("	new expression: |" + newExpression + "|");
#endif

	bool addStrings = false;
	for (int i = 0; i < (int)newExpression.size(); i++)
		if (isalpha(newExpression[i]) || (newExpression[i] == '\"' && !isEscaped(newExpression, i)))
		{
			addStrings = true;
			break;
		}
	if (addStrings)
	{
		inQuotes = false;
		string withoutParenthesis = "";
		for (int i = 0; i < (int)newExpression.size(); i++)
		{
			if (newExpression[i] == '\"' && !isEscaped(newExpression, i))
			{
				inQuotes = !inQuotes;
				continue;
			}
			if (inQuotes)
				withoutParenthesis += newExpression[i];
			if (!inQuotes && newExpression[i] != '(' && newExpression[i] != ')' && newExpression[i] != '+' && newExpression[i] != ' ')
				withoutParenthesis += newExpression[i];
		}

		//cout << "NewSTRING = " << Quoted(withoutParenthesis) << endl;
		return withoutParenthesis;
	}
	else
		return evaluate(newExpression);
}

bool BooleanLogic(const string& valA, const string& determinant, const string& valB, unordered_map<string, boost::any>& variableValues)
{
	boost::any valARealValue = EvalExpression(valA, variableValues);
	boost::any valBRealValue = EvalExpression(valB, variableValues);
#if DEVELOPER_MESSAGES == true
	InterpreterLog(AnyAsString(valARealValue) + " " + determinant + " " + AnyAsString(valBRealValue) + " : " + AnyAsString(valA) + " " + determinant + " " + AnyAsString(valB) + " : " + to_string(AnyAsString(valARealValue) == AnyAsString(valBRealValue)));
#endif
	if (determinant == "==")
		return any_compare(valARealValue, valBRealValue);
	else if (determinant == "!=")
		return !any_compare(valARealValue, valBRealValue);
	else if (determinant == ">=")
		return AnyAsFloat(valARealValue) >= AnyAsFloat(valBRealValue);
	else if (determinant == "<=")
		return AnyAsFloat(valARealValue) <= AnyAsFloat(valBRealValue);
	else if (determinant == ">")
		return AnyAsFloat(valARealValue) > AnyAsFloat(valBRealValue);
	else if (determinant == "<")
		return AnyAsFloat(valARealValue) < AnyAsFloat(valBRealValue);
	else
		LogWarning("unrecognized determinant \'" + determinant + "\'");

	return false;
}

int varOperation(const vector<string>& str, unordered_map<string, boost::any>& variableValues)
{
	if (IsVar(str.at(0), variableValues))
	{
		// Checks if type is simple, like int or string
		if (any_type(variableValues[str.at(0)]) <= 3)
		{
			if (str.at(1) == "=")
				variableValues[str.at(0)] = EvalExpression(unWrapVec(vector<string>(str.begin() + 2, str.end())), variableValues);
			else if (str.at(1) == "+=")
				variableValues[str.at(0)] = EvalExpression(str.at(0) + "+(" + unWrapVec(vector<string>(str.begin() + 2, str.end())) + ")", variableValues);
			else if (str.at(1) == "-=")
				variableValues[str.at(0)] = AnyAsFloat(variableValues[str.at(0)]) - AnyAsFloat(EvalExpression(unWrapVec(vector<string>(str.begin() + 2, str.end())), variableValues));
			else if (str.at(1) == "*=")
				variableValues[str.at(0)] = AnyAsFloat(variableValues[str.at(0)]) * AnyAsFloat(EvalExpression(unWrapVec(vector<string>(str.begin() + 2, str.end())), variableValues));
			else if (str.at(1) == "/=")
				variableValues[str.at(0)] = AnyAsFloat(variableValues[str.at(0)]) / AnyAsFloat(EvalExpression(unWrapVec(vector<string>(str.begin() + 2, str.end())), variableValues));
			else
				LogWarning("unrecognized operator \'" + str.at(1) + "\'");
		}
		// Else it is a Vec2. No other complex class can be operated on it's base form (ex. you can't do: Sprite += Sprite)
		else if (any_type(variableValues[str.at(0)]) == 5)
		{
			boost::any otherExpression = EvalExpression(unWrapVec(vector<string>(str.begin() + 2, str.end())), variableValues);
			if (str.at(1) == "=")
				variableValues[str.at(0)] = otherExpression;
			else if (str.at(1) == "+=")
				variableValues[str.at(0)] = AnyAsVec2(variableValues[str.at(0)]) + AnyAsVec2(otherExpression);
			else if (str.at(1) == "-=")
				variableValues[str.at(0)] = AnyAsVec2(variableValues[str.at(0)]) - AnyAsVec2(otherExpression);
			else if (str.at(1) == "*=")
				variableValues[str.at(0)] = AnyAsVec2(variableValues[str.at(0)]) * AnyAsFloat(otherExpression);
			else if (str.at(1) == "/=")
				variableValues[str.at(0)] = AnyAsVec2(variableValues[str.at(0)]) / AnyAsFloat(otherExpression);
			else
				LogWarning("unrecognized operator \'" + str.at(1) + "\'");
		}
		return 0;
	}
	else if (IsVar(str.at(0), globalVariableValues))
	{
		// Checks if type is simple, like int or string
		if (any_type(globalVariableValues[str.at(0)]) <= 3)
		{
			if (str.at(1) == "=")
				globalVariableValues[str.at(0)] = EvalExpression(unWrapVec(vector<string>(str.begin() + 2, str.end())), variableValues);
			else if (str.at(1) == "+=")
				globalVariableValues[str.at(0)] = EvalExpression(str.at(0) + "+(" + unWrapVec(vector<string>(str.begin() + 2, str.end())) + ")", variableValues);
			else if (str.at(1) == "-=")
				globalVariableValues[str.at(0)] = AnyAsFloat(globalVariableValues[str.at(0)]) - AnyAsFloat(EvalExpression(unWrapVec(vector<string>(str.begin() + 2, str.end())), variableValues));
			else if (str.at(1) == "*=")
				globalVariableValues[str.at(0)] = AnyAsFloat(globalVariableValues[str.at(0)]) * AnyAsFloat(EvalExpression(unWrapVec(vector<string>(str.begin() + 2, str.end())), variableValues));
			else if (str.at(1) == "/=")
				globalVariableValues[str.at(0)] = AnyAsFloat(globalVariableValues[str.at(0)]) / AnyAsFloat(EvalExpression(unWrapVec(vector<string>(str.begin() + 2, str.end())), variableValues));
			else
				LogWarning("unrecognized operator \'" + str.at(1) + "\'");
		}
		// Else it is a Vec2. No other complex class can be operated on it's base form (ex. you can't do: Sprite += Sprite)
		else if (any_type(globalVariableValues[str.at(0)]) == 5)
		{
			boost::any otherExpression = EvalExpression(unWrapVec(vector<string>(str.begin() + 2, str.end())), variableValues);
			if (str.at(1) == "=")
				globalVariableValues[str.at(0)] = otherExpression;
			else if (str.at(1) == "+=")
				globalVariableValues[str.at(0)] = AnyAsVec2(globalVariableValues[str.at(0)]) + AnyAsVec2(otherExpression);
			else if (str.at(1) == "-=")
				globalVariableValues[str.at(0)] = AnyAsVec2(globalVariableValues[str.at(0)]) - AnyAsVec2(otherExpression);
			else if (str.at(1) == "*=")
				globalVariableValues[str.at(0)] = AnyAsVec2(globalVariableValues[str.at(0)]) * AnyAsFloat(otherExpression);
			else if (str.at(1) == "/=")
				globalVariableValues[str.at(0)] = AnyAsVec2(globalVariableValues[str.at(0)]) / AnyAsFloat(otherExpression);
			else
				LogWarning("unrecognized operator \'" + str.at(1) + "\'");
		}
		return 0;
	}
	LogWarning("uninitialized variable or typo in \'" + str.at(0) + "\'");
	return 1;
}

boost::any ProcessLine(const vector<vector<string>>& words, int lineNum, unordered_map<string, boost::any>& variableValues)
{
	// Check if the first two chars are '//', which would make it a comment
	if (words.at(lineNum).at(0)[0] == '/' && words.at(lineNum).at(0)[1] == '/')
		return nullType;

	// If print statement (deprecated, now use ZS.System.Print() function)
	else if (words.at(lineNum).at(0) == "print")
	{
		cout << StringRaw(AnyAsString(EvalExpression(unWrapVec(vector<string>(words.at(lineNum).begin() + 1, words.at(lineNum).end())), variableValues))) << endl;
		return nullType;
	}

	// Check if it is a function return
	else if (words.at(lineNum).at(0) == "return")
		return EvalExpression(unWrapVec(vector<string>(words.at(lineNum).begin() + 1, words.at(lineNum).end())), variableValues);

	// Check if it is ZS Builtin function
	else if (words.at(lineNum).at(0)[0] == 'Z' && words.at(lineNum).at(0)[1] == 'S' && words.at(lineNum).at(0)[2] == '.')
		return EvalExpression(unWrapVec(words.at(lineNum)), variableValues);

	// Check if it is function
	else if (IsFunction(trim(split(words.at(lineNum).at(0), '(')[0])))
	{
		if (count(words.at(lineNum).at(0), '(') > 0 && count(words.at(lineNum).at(0), ')') > 0)
			ExecuteFunction(trim(split(words.at(lineNum).at(0), '(')[0]), vector<boost::any>());
		else
			ExecuteFunction(trim(split(words.at(lineNum).at(0), '(')[0]), VarValues(split(RMParenthesis("(" + split(unWrapVec(rangeInVec(words.at(lineNum), 0, (int)words.at(lineNum).size() - 1)), '(')[1]), ','), variableValues));
		return nullType;
	}

	// Check if global variable declaration
	else if (trim(words.at(lineNum).at(0)) == "global")
	{
		globalVariableValues[words.at(lineNum).at(2)] = EvalExpression(unWrapVec(slice(words.at(lineNum), 4, -1)), variableValues);
		return nullType;
	}

	// Iterate through all types to see if line inits or
	// re-inits a variable then store it with it's value
	else if (countInVector(types, trim(words.at(lineNum).at(0))) > 0)
	{
		variableValues[words.at(lineNum).at(1)] = EvalExpression(unWrapVec(slice(words.at(lineNum), 3, -1)), variableValues);
		return nullType;
	}

	// Check existing variables: If matches, then it means
	// the variables value is getting changed with an operator
	else if (count(words.at(lineNum).at(0), '.') == 0 && (IsVar(words.at(lineNum).at(0), variableValues) || IsVar(words.at(lineNum).at(0), globalVariableValues)))
	{
		// Evaluates what the operator (ex. '=', '+=') does to the value on the left by the value on the right
		varOperation(vector<string>(words.at(lineNum).begin(), words.at(lineNum).end()), variableValues);
		return nullType;
	}

	// Check existing variables: To see if accessign class sub component
	else if (count(words.at(lineNum).at(0), '.') > 0 && IsVar(split(words.at(lineNum).at(0), '.')[0], variableValues) || IsVar(split(words.at(lineNum).at(0), '.')[0], globalVariableValues))
	{
		if (IsVar(split(words.at(lineNum).at(0), '.')[0], variableValues))
			variableValues[split(words.at(lineNum).at(0), '.')[0]] = EditClassSubComponent(variableValues[split(words.at(lineNum).at(0), '.')[0]], words.at(lineNum).at(1), EvalExpression(unWrapVec(vector<string>(words.at(lineNum).begin() + 2, words.at(lineNum).end())), variableValues), split(words.at(lineNum).at(0), '.')[1]);
		else if (IsVar(split(words.at(lineNum).at(0), '.')[0], globalVariableValues))
			globalVariableValues[split(words.at(lineNum).at(0), '.')[0]] = EditClassSubComponent(globalVariableValues[split(words.at(lineNum).at(0), '.')[0]], words.at(lineNum).at(1), EvalExpression(unWrapVec(vector<string>(words.at(lineNum).begin() + 2, words.at(lineNum).end())), variableValues), split(words.at(lineNum).at(0), '.')[1]);
		return nullType;
	}

	// If declaring a while loop, loop until false
	else if (words.at(lineNum).at(0) == "while")
	{
		vector<vector<string>> whileContents;
		vector<string> whileParameters;

		for (int w = 1; w < (int)words.at(lineNum).size(); w++)
			whileParameters.push_back(words.at(lineNum)[w]);

		int numOfBrackets = 1;
		for (int p = lineNum + 2; p < (int)words.size(); p++)
		{
			numOfBrackets += countInVector(words.at(p), "{") - countInVector(words.at(p), "}");
			if (numOfBrackets == 0)
				break;
			whileContents.push_back(words.at(p));
		}
		whileContents = removeTabsWdArry(whileContents, 1);

		while (BooleanLogic(whileParameters.at(0), whileParameters.at(1), whileParameters.at(2), variableValues))
		{
			//Iterate through all lines in while loop
			for (int lineNum = 0; lineNum < (int)whileContents.size(); lineNum++)
			{
				boost::any returnVal = ProcessLine(whileContents, lineNum, variableValues);
				if (!returnVal.empty())
					return returnVal;
			}
		}
		return nullType;
	}

	// If declaring an if statement, only execute if true
	else if (words.at(lineNum).at(0) == "if")
	{
		vector<vector<string>> ifContents;
		vector<string> ifParameters;

		for (int w = 1; w < (int)words.at(lineNum).size(); w++)
			ifParameters.push_back(words.at(lineNum).at(w));

		int numOfBrackets = 1;
		lineNum += 2;
		while (lineNum < (int)words.size())
		{
			numOfBrackets += countInVector(words.at(lineNum), "{") - countInVector(words.at(lineNum), "}");
			if (numOfBrackets == 0)
				break;
			ifContents.push_back(words.at(lineNum));
			lineNum++;
		}
		ifContents = removeTabsWdArry(ifContents, 1);

		if (BooleanLogic(ifParameters.at(0), ifParameters.at(1), ifParameters.at(2), variableValues))
		{
			//Iterate through all lines in if statement
			for (int l = 0; l < (int)ifContents.size(); l++)
			{
				boost::any returnVal = ProcessLine(ifContents, l, variableValues);
				if (!returnVal.empty())
					return returnVal;
			}
		}
		//else if (words.size() > lineNum + 1)
		//	if (words[lineNum + 1][0] == "else")
		//	{
		//		lineNum += 1;

		//		vector<string> elseContents;

		//		int numOfBrackets = 1;
		//		while (lineNum < (int)words.size())
		//		{
		//			numOfBrackets += countInVector(words[lineNum], "{") - countInVector(words[lineNum], "}");
		//			if (numOfBrackets == 0)
		//				break;
		//			elseContents.push_back("");
		//			for (int w = 0; w < (int)words[lineNum].size(); w++)
		//			{
		//				elseContents[(int)elseContents.size() - 1] += words[lineNum][w] + " ";
		//			}
		//			lineNum++;
		//		}
		//		elseContents = removeTabs(elseContents, 2);

		//		vector<vector<string>> innerWords;
		//		for (int i = 0; i < (int)elseContents.size(); i++)
		//			innerWords.push_back(split(elseContents[i], ' '));

		//		//Iterate through all lines in else statement
		//		for (int lineNum = 0; lineNum < (int)elseContents.size(); lineNum++)
		//		{
		//			ProcessLine(innerWords, lineNum, variableValues);
		//		}
		//		return nullType;
		//	}
		return nullType;
	}
	//// Gathers else statement contents
	//if (words[lineNum][0] == "else")
	//{
	//	
	//}

	return nullType;
}

boost::any ExecuteFunction(const string& functionName, const vector<boost::any>& inputVarVals)
{
	// Get contents of function from global function map
	std::vector<std::vector<std::string>> words = functionValues[functionName];

	unordered_map<string, boost::any> variableValues = {};

	std::vector<std::string> funcArgs = words.at(0);
	// Set function variables equal to whatever inputs were provided
	for (int i = 0; i < (int)inputVarVals.size(); i++)
	{
		if (i < funcArgs.size())
		{
			variableValues[funcArgs[i]] = inputVarVals[i];
#if DEVELOPER_MESSAGES == true
			cout << functionName + "  \x1B[33m" << funcArgs[i] << " == " << AnyAsString(inputVarVals[i]) << "\033[0m\t\t" << endl;
#endif
		}
	}

	//Iterate through all lines in function
	for (int lineNum = 1; lineNum < (int)words.size(); lineNum++)
	{
		try
		{
			boost::any returnVal = ProcessLine(words, lineNum, variableValues);
			if (!returnVal.empty())
				return returnVal;
		}
		catch (const std::exception&)
		{
		}
	}
	return nullType;
}

int parseZSharp(string script)
{
	script = replace(script, "    ", "\t"); // Replace spaces with tabs (not really required, and will break purposefull whitespace in strings etc.)
	#if DEVELOPER_MESSAGES
	InterpreterLog("Contents:\n" + script);
#endif

	// Split the script by ;, signifying a line ending
	vector<string> lines = split(script, '\n');
	vector<vector<string>> words;
	for (int i = 0; i < (int)lines.size(); i++) // Then split said lines into indiviual words
		words.push_back(split(lines.at(i), ' '));

#if DEVELOPER_MESSAGES
	InterpreterLog("Gather variables & functions...");
#endif
	// First go through entire script and iterate through all types to see if line is a variable
	// or function declaration, then store it with it's value
	for (int lineNum = 0; lineNum < (int)words.size(); lineNum++)
	{
		//Checks if it is function
		if (words.at(lineNum).at(0) == "func")
		{
			vector<vector<string>> functionContents;

			string functName = split(words.at(lineNum).at(1), '(')[0];
#if DEVELOPER_MESSAGES == true
			InterpreterLog("Load script function " + functName + "...");
#endif

			string args = "";
			if (indexInStr(unWrapVec(words.at(lineNum)), ')') - indexInStr(unWrapVec(words.at(lineNum)), '(') > 1)
				for (int w = 1; w < (int)words.at(lineNum).size(); w++) // Get all words from the instantiation line: these are the args
				{
					args += replace(replace(words.at(lineNum).at(w), "(", " "), ")", "");
				}

			args = trim(replace(args, functName + " ", ""));
			functionContents.push_back(split(args, ','));

			int numOfBrackets = 1;
			for (int p = lineNum + 2; p < (int)words.size(); p++)
			{
				numOfBrackets += countInVector(words.at(p), "{") - countInVector(words.at(p), "}");
				if (numOfBrackets == 0)
					break;
				functionContents.push_back(removeTabs(words.at(p), 1));
			}
			functionValues[functName] = functionContents;
		}
		else
		{
			if (words.at(lineNum).at(0) == "string") {
				globalVariableValues[words.at(lineNum).at(1)] = StringRaw(words.at(lineNum).at(3));
#if DEVELOPER_MESSAGES == true
				InterpreterLog("Load script variable " + words.at(lineNum).at(1) + "...");
#endif
			}
			else if (words.at(lineNum).at(0) == "int") {
				globalVariableValues[words.at(lineNum).at(1)] = stoi(words.at(lineNum).at(3));
#if DEVELOPER_MESSAGES == true
				InterpreterLog("Load script variable " + words.at(lineNum).at(1) + "...");
#endif
			}
			else if (words.at(lineNum).at(0) == "float") {
				globalVariableValues[words.at(lineNum).at(1)] = stof(words.at(lineNum).at(3));
#if DEVELOPER_MESSAGES == true
				InterpreterLog("Load script variable " + words.at(lineNum).at(1) + "...");
#endif
			}
			else if (words.at(lineNum).at(0) == "bool") {
				globalVariableValues[words.at(lineNum).at(1)] = stob(words.at(lineNum).at(3));
#if DEVELOPER_MESSAGES == true
				InterpreterLog("Load script variable " + words.at(lineNum).at(1) + "...");
#endif
			}
			/*else
				LogWarning("unrecognized type \'" + words.at(lineNum).at(0) + "\' on line: " + to_string(lineNum));*/
		}
	}

#if DEVELOPER_MESSAGES
	InterpreterLog("Start Main()");
	#endif
	// Executes main, which is the entry point function
	ExecuteFunction("Main", vector<boost::any> {});

	return 0;
}

int main(int argc, char* argv[])
{
	// Print the name of the interpreter and it's version in inverted black on white text
	PrintColored(NAMEVERSION, blackFGColor, whiteBGColor, false);
	cout << endl << endl;

	// Gathers builtin functions and variables
	GetBuiltins(ZSContents);
	functionValues = builtinFunctionValues;
	globalVariableValues = builtinVarVals;

	std::string scriptTextContents;

	// If scriptname is supplied and not in developer mode
	if (argc > 1 || EXAMPLE_PROJECT)
	{
		std::string scriptPath;
		if (EXAMPLE_PROJECT)
			scriptPath = "D:\\Code\\Z-Sharp\\Releases\\ZS-Win-x64-Base\\Pong-Example-Project\\script.zs";
		else
			scriptPath = argv[1];
#if DEVELOPER_MESSAGES
		cout << scriptPath << endl;
#endif

		std::string projectDirectory = scriptPath.substr(0, scriptPath.find_last_of("/\\"));
#if UNIX
		// Get script contents as single string
		auto ss = ostringstream{};
		ifstream input_file(scriptPath);
		ss << input_file.rdbuf();
		scriptTextContents = ss.str();
#if DEVELOPER_MESSAGES
		InterpreterLog("Gather script contents...");
		#endif
		
		// Change the current working directory to that of the script
		chdir(projectDirectory.c_str());
#if DEVELOPER_MESSAGES
		InterpreterLog("Change directory to " + projectDirectory + "...");
#endif
#if DEVELOPER_MESSAGES
		string newPath = filesystem::current_path();
		InterpreterLog("Current working directory is " + newPath);
#endif
#elif WINDOWS
		// Get script contents as single string
		ifstream script(scriptPath);
		stringstream scriptString;
		scriptString << script.rdbuf();
		scriptTextContents = scriptString.str();
#if DEVELOPER_MESSAGES
		InterpreterLog("Gather script contents...");
		#endif
		
		// Change the current working directory to that of the script
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		std::wstring wide = converter.from_bytes(projectDirectory);
		LPCWSTR s = wide.c_str();
		SetCurrentDirectory(s);
#if DEVELOPER_MESSAGES
		InterpreterLog("Change directory to " + projectDirectory + "...");
#endif
#endif
	}
	else
	{ // If no script is provided as an argument then throw error
		LogWarning("No script provided! Please drag and drop .ZS file over interpreter executable file, or provide it's path as a command-line argument.");
		cout << "Press Enter to Continue";
		cin.ignore();
		exit(1);
	}

	#if DEVELOPER_MESSAGES
	InterpreterLog("Parsing...");
	#endif
	// Start running the script
	parseZSharp(scriptTextContents);
	
	// Entire script has been run, exit.
	
#if DEVELOPER_MESSAGES // If built with developer messages, then verify exit
	cout << "Press Enter to Continue";
	cin.ignore();
	exit(1);
#else
	if(argc > 2)
	{
		string a = argv[2];
		std::transform(a.begin(), a.end(), a.begin(),
    			[](unsigned char c){ return std::tolower(c); });
		
		if(a == "-ve") // If the '-ve' (verify exit) option is used, ask for verification on exit
		{
			cout << "Press Enter to Continue";
			cin.ignore();
			exit(1);	
		}
	}
#endif // Else exit automatically
	return 0;
}
