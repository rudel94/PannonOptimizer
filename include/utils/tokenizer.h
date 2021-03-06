//=================================================================================================
/*!
//  This file is part of the Pannon Optimizer library. 
//  This library is free software; you can redistribute it and/or modify it under the 
//  terms of the GNU Lesser General Public License as published by the Free Software 
//  Foundation; either version 3.0, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
//  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//  See the GNU General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License; see the file 
//  COPYING. If not, see http://www.gnu.org/licenses/.
*/
//=================================================================================================

/**
 * @file tokenizer.h This file contains the API of the Tokenizer class.
 * @author
 */

#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <globals.h>
#include <utils/exceptions.h>
#include <iostream>
#include <vector>
#include <string>
#include <debug.h>

/**
 * This class is able to divide an input file into tokens.
 * With tokens the organizing and debugging of input data is easier.
 * This class functions as a DFA with states describing the actual sequence type in
 * the input file.
 *
 * @class Tokenizer
 */
class Tokenizer {
public:

    /**
     * This struct describes a token of the tokenized file.
     */
    struct Token {
        std::string m_value;
        unsigned int m_row;
        unsigned int m_column;
    };

    /**
     * Tokenizes a specified input stream.
     *
     * @param is The input stream to be tokenized.
     * @param tokens Pointer to the output token array.
     * @param rows Pointer to the output row array.
     */
    static void tokenize(std::istream & is, std::vector< Token > * tokens,
                         std::vector<std::string> * rows);
private:

    /**
     * This type describes the state of the Tokenizer object.
     * The state decides how to handle the next input characters.
     */
    enum TOKENIZER_STATE {
        TOKENIZER_START,
        TOKENIZER_COMMENT,
        TOKENIZER_WORD,
        TOKENIZER_NUMBER,
        TOKENIZER_NUMBER_WIDTH_DOT,
       // TOKENIZER_SPECIAL,
        TOKENIZER_STRING,
        TOKENIZER_ESCAPE_SEQ,
        TOKENIZER_WHITESPACE,
        TOKENIZER_ELLIPSIS_OR_NUMBER,
        TOKENIZER_ELLIPSIS_2,
      //  TOKENIZER_ELLIPSIS_3,
        TOKENIZER_END,
        TOKENIZER_ERROR,
        TOKENIZER_STATE_COUNT
    };

    /**
     * Initializes the state table of the tokenizer, which implements its DFA functionality.
     *
     * @param tablePtr Pointer to the vector of possible states.
     * @param transitionTablePtr Pointer to the vector of transitions between states.
     */
    static void initStateTable(std::vector< std::vector<TOKENIZER_STATE> > * tablePtr,
                               std::vector<std::vector<std::string> > * transitionTablePtr);

    /**
     * Returns a given tokenizer state in a human readale format.
     *
     * @param state The state to be converted.
     * @return The state in a human readale format.
     */
    static std::string stateToString(TOKENIZER_STATE state);
};

#endif // TOKENIZER_H
