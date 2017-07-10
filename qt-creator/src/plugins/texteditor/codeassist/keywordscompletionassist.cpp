/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "keywordscompletionassist.h"

#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/codeassist/functionhintproposal.h>
#include <texteditor/codeassist/genericproposalmodel.h>
#include <texteditor/completionsettings.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/texteditor.h>
//OPENMV-DIFF//
#include <QRegularExpression>
#include <QStack>
//OPENMV-DIFF//

namespace TextEditor {

// --------------------------
// Keywords
// --------------------------
Keywords::Keywords()
{
}

// Note: variables and functions must be sorted
Keywords::Keywords(const QStringList &variabels, const QStringList &functions, const QMap<QString, QStringList> &functionArgs)
    : m_variables(variabels), m_functions(functions), m_functionArgs(functionArgs)
{

}

bool Keywords::isVariable(const QString &word) const
{
    //OPENMV-DIFF//
    //return qBinaryFind(m_variables, word) != m_variables.constEnd();
    //OPENMV-DIFF//
    return m_variables.contains(word);
    //OPENMV-DIFF//
}

bool Keywords::isFunction(const QString &word) const
{
    //OPENMV-DIFF//
    //return qBinaryFind(m_functions, word) != m_functions.constEnd();
    //OPENMV-DIFF//
    return m_functions.contains(word);
    //OPENMV-DIFF//
}

QStringList Keywords::variables() const
{
    return m_variables;
}

QStringList Keywords::functions() const
{
    return m_functions;
}

QStringList Keywords::argsForFunction(const QString &function) const
{
    return m_functionArgs.value(function);
}


// --------------------------
// KeywordsAssistProposalItem
// --------------------------
KeywordsAssistProposalItem::KeywordsAssistProposalItem(bool isFunction)
    : m_isFunction(isFunction)
{
}

KeywordsAssistProposalItem::~KeywordsAssistProposalItem() Q_DECL_NOEXCEPT
{}

bool KeywordsAssistProposalItem::prematurelyApplies(const QChar &c) const
{
    // only '(' in case of a function
    return c == QLatin1Char('(') && m_isFunction;
}

void KeywordsAssistProposalItem::applyContextualContent(TextDocumentManipulatorInterface &manipulator,
                                                        int basePosition) const
{
    const CompletionSettings &settings = TextEditorSettings::completionSettings();

    int replaceLength = manipulator.currentPosition() - basePosition;
    QString toInsert = text();
    int cursorOffset = 0;
    const QChar characterAtCurrentPosition = manipulator.characterAt(manipulator.currentPosition());

    if (m_isFunction && settings.m_autoInsertBrackets) {
        if (settings.m_spaceAfterFunctionName) {
            if (manipulator.textAt(manipulator.currentPosition(), 2) == QLatin1String(" (")) {
                cursorOffset = 2;
            } else if ( characterAtCurrentPosition == QLatin1Char('(')
                       || characterAtCurrentPosition == QLatin1Char(' ')) {
                replaceLength += 1;
                toInsert += QLatin1String(" (");
            } else {
                toInsert += QLatin1String(" ()");
                cursorOffset = -1;
            }
        } else {
            if (characterAtCurrentPosition == QLatin1Char('(')) {
                cursorOffset = 1;
            } else {
                toInsert += QLatin1String("()");
                cursorOffset = -1;
            }
        }
    }

    manipulator.replace(basePosition, replaceLength, toInsert);
    if (cursorOffset)
        manipulator.setCursorPosition(manipulator.currentPosition() + cursorOffset);
}

// -------------------------
// KeywordsFunctionHintModel
// -------------------------
KeywordsFunctionHintModel::KeywordsFunctionHintModel(const QStringList &functionSymbols)
        : m_functionSymbols(functionSymbols)
{}

KeywordsFunctionHintModel::~KeywordsFunctionHintModel()
{}

void KeywordsFunctionHintModel::reset()
{}

int KeywordsFunctionHintModel::size() const
{
    return m_functionSymbols.size();
}

QString KeywordsFunctionHintModel::text(int index) const
{
    return m_functionSymbols.at(index);
}

int KeywordsFunctionHintModel::activeArgument(const QString &prefix) const
{
    //OPENMV-DIFF//
    {
        enum
        {
            IN_PUSH_0, // (
            IN_PUSH_1, // [
            IN_PUSH_2, // {
            IN_COMMA
        };

        QStack<int> in_stack;

        enum
        {
            IN_NONE,
            IN_COMMENT,
            IN_STRING_0,
            IN_STRING_1
        }
        in_state = IN_NONE;

        for(int i = 0; i < prefix.size(); i++)
        {
            switch(in_state)
            {
                case IN_NONE:
                {
                    if((prefix.at(i) == QLatin1Char('#')) && ((!i) || (prefix.at(i-1) != QLatin1Char('\\')))) in_state = IN_COMMENT;
                    if((prefix.at(i) == QLatin1Char('\'')) && ((!i) || (prefix.at(i-1) != QLatin1Char('\\')))) in_state = IN_STRING_0;
                    if((prefix.at(i) == QLatin1Char('\"')) && ((!i) || (prefix.at(i-1) != QLatin1Char('\\')))) in_state = IN_STRING_1;
                    if(prefix.at(i) == QLatin1Char('(')) in_stack.push(IN_PUSH_0);
                    if(prefix.at(i) == QLatin1Char('[')) in_stack.push(IN_PUSH_1);
                    if(prefix.at(i) == QLatin1Char('{')) in_stack.push(IN_PUSH_2);
                    if(prefix.at(i) == QLatin1Char(')')) while(in_stack.size() && (in_stack.pop() != IN_PUSH_0));
                    if(prefix.at(i) == QLatin1Char(']')) while(in_stack.size() && (in_stack.pop() != IN_PUSH_1));
                    if(prefix.at(i) == QLatin1Char('}')) while(in_stack.size() && (in_stack.pop() != IN_PUSH_2));
                    if(prefix.at(i) == QLatin1Char(',')) in_stack.push(IN_COMMA);
                    break;
                }
                case IN_COMMENT:
                {
                    if((prefix.at(i) == QLatin1Char('\n')) && (prefix.at(i-1) != QLatin1Char('\\'))) in_state = IN_NONE;
                    break;
                }
                case IN_STRING_0:
                {
                    if((prefix.at(i) == QLatin1Char('\'')) && (prefix.at(i-1) != QLatin1Char('\\'))) in_state = IN_NONE;
                    break;
                }
                case IN_STRING_1:
                {
                    if((prefix.at(i) == QLatin1Char('\"')) && (prefix.at(i-1) != QLatin1Char('\\'))) in_state = IN_NONE;
                    break;
                }
            }
        }

        int commaCount = 0;
        while(in_stack.size() && (in_stack.top() == IN_COMMA))
        {
            commaCount += 1;
            in_stack.pop();
        }

        return ((in_stack.size() == 1) && (in_stack.top() == IN_PUSH_0) && (commaCount < m_functionSymbols.size())) ? commaCount : -1;
    }
    //OPENMV-DIFF//
    Q_UNUSED(prefix);
    return 1;
}

// ---------------------------------
// KeywordsCompletionAssistProcessor
// ---------------------------------
KeywordsCompletionAssistProcessor::KeywordsCompletionAssistProcessor(Keywords keywords)
    : m_startPosition(-1)
    , m_variableIcon(QLatin1String(":/codemodel/images/keyword.png"))
    , m_functionIcon(QLatin1String(":/codemodel/images/func.png"))
    , m_keywords(keywords)
{}

KeywordsCompletionAssistProcessor::~KeywordsCompletionAssistProcessor()
{}

IAssistProposal *KeywordsCompletionAssistProcessor::perform(const AssistInterface *interface)
{
    m_interface.reset(interface);
    //OPENMV-DIFF//
    {
        QTextCursor cursor(m_interface->textDocument());
        cursor.setPosition(m_interface->position());
        cursor.movePosition(QTextCursor::Start, QTextCursor::KeepAnchor);
        QString text = cursor.selectedText().replace(QChar::ParagraphSeparator, QLatin1Char('\n'));

        enum
        {
            IN_PUSH_0, // (
            IN_PUSH_1, // [
            IN_PUSH_2, // {
            IN_COMMA
        };

        QStack< QPair<int, int> > in_stack;

        enum
        {
            IN_NONE,
            IN_COMMENT,
            IN_STRING_0,
            IN_STRING_1
        }
        in_state = IN_NONE;

        for(int i = 0; i < text.size(); i++)
        {
            switch(in_state)
            {
                case IN_NONE:
                {
                    if((text.at(i) == QLatin1Char('#')) && ((!i) || (text.at(i-1) != QLatin1Char('\\')))) in_state = IN_COMMENT;
                    if((text.at(i) == QLatin1Char('\'')) && ((!i) || (text.at(i-1) != QLatin1Char('\\')))) in_state = IN_STRING_0;
                    if((text.at(i) == QLatin1Char('\"')) && ((!i) || (text.at(i-1) != QLatin1Char('\\')))) in_state = IN_STRING_1;
                    if(text.at(i) == QLatin1Char('(')) in_stack.push(QPair<int, int>(IN_PUSH_0, i));
                    if(text.at(i) == QLatin1Char('[')) in_stack.push(QPair<int, int>(IN_PUSH_1, i));
                    if(text.at(i) == QLatin1Char('{')) in_stack.push(QPair<int, int>(IN_PUSH_2, i));
                    if(text.at(i) == QLatin1Char(')')) while(in_stack.size() && (in_stack.pop().first != IN_PUSH_0));
                    if(text.at(i) == QLatin1Char(']')) while(in_stack.size() && (in_stack.pop().first != IN_PUSH_1));
                    if(text.at(i) == QLatin1Char('}')) while(in_stack.size() && (in_stack.pop().first != IN_PUSH_2));
                    if(text.at(i) == QLatin1Char(',')) in_stack.push(QPair<int, int>(IN_COMMA, i));
                    break;
                }
                case IN_COMMENT:
                {
                    if((text.at(i) == QLatin1Char('\n')) && (text.at(i-1) != QLatin1Char('\\'))) in_state = IN_NONE;
                    break;
                }
                case IN_STRING_0:
                {
                    if((text.at(i) == QLatin1Char('\'')) && (text.at(i-1) != QLatin1Char('\\'))) in_state = IN_NONE;
                    break;
                }
                case IN_STRING_1:
                {
                    if((text.at(i) == QLatin1Char('\"')) && (text.at(i-1) != QLatin1Char('\\'))) in_state = IN_NONE;
                    break;
                }
            }
        }

        if(in_state != IN_NONE) return 0;

        while(in_stack.size() && (in_stack.top().first == IN_COMMA))
        {
            in_stack.pop();
        }

        if(!m_interface->position()) return 0;
        QChar chr = m_interface->characterAt(m_interface->position() - 1);

        if(chr == QLatin1Char('.'))
        {
            if(!(m_interface->position() - 1)) return 0;
            cursor.setPosition(m_interface->position() - 2);
            cursor.select(QTextCursor::WordUnderCursor);
            if(cursor.selectedText().isEmpty()) return 0;
            if(!cursor.selectedText().contains(QRegularExpression(QStringLiteral("[a-zA-Z_][a-zA-Z_0-9]*")))) return 0;
            m_startPosition = m_interface->position();

            QList<AssistProposalItemInterface *> items;
            addWordsToProposalList(&items, m_keywords.variables(), m_variableIcon);
            addWordsToProposalList(&items, m_keywords.functions(), m_functionIcon);
            return new GenericProposal(m_startPosition, items);
        }
        else if(chr == QLatin1Char('('))
        {
            if(!(m_interface->position() - 1)) return 0;
            cursor.setPosition(m_interface->position() - 2);
            cursor.select(QTextCursor::WordUnderCursor);
            if(!m_keywords.isFunction(cursor.selectedText())) return 0;
            m_startPosition = m_interface->position() - cursor.selectedText().size() - 1;

            m_word = cursor.selectedText();
            return new FunctionHintProposal(m_startPosition,
                new KeywordsFunctionHintModel((m_keywords.functions().count(m_word) > 1) ? QStringList() : m_keywords.argsForFunction(m_word)));
        }
        else if((chr == QLatin1Char(',')) && (in_stack.size() >= 1) && (in_stack.top().first == IN_PUSH_0))
        {
            if(!in_stack.top().second) return 0;
            cursor.setPosition(in_stack.top().second - 1);
            cursor.select(QTextCursor::WordUnderCursor);
            if(!m_keywords.isFunction(cursor.selectedText())) return 0;
            m_startPosition = in_stack.top().second - cursor.selectedText().size();

            m_word = cursor.selectedText();
            return new FunctionHintProposal(m_startPosition,
                new KeywordsFunctionHintModel((m_keywords.functions().count(m_word) > 1) ? QStringList() : m_keywords.argsForFunction(m_word)));
        }
        else if(chr.isLetterOrNumber() || (chr == QLatin1Char('_')))
        {
            cursor.setPosition(m_interface->position() - 1);
            cursor.select(QTextCursor::WordUnderCursor);
            if(cursor.selectedText().isEmpty()) return 0;
            if(!cursor.selectedText().contains(QRegularExpression(QStringLiteral("[a-zA-Z_][a-zA-Z_0-9]*")))) return 0;
            m_startPosition = m_interface->position() - cursor.selectedText().size();

            QList<AssistProposalItemInterface *> items;
            addWordsToProposalList(&items, m_keywords.variables(), m_variableIcon);
            addWordsToProposalList(&items, m_keywords.functions(), m_functionIcon);
            return new GenericProposal(m_startPosition, items);
        }

        return 0;
    }
    //OPENMV-DIFF//
    if (isInComment())
        return 0;

    if (interface->reason() == IdleEditor && !acceptsIdleEditor())
        return 0;

    if (m_startPosition == -1)
        m_startPosition = findStartOfName();

    int nextCharPos = m_startPosition + m_word.length();
    if (m_keywords.isFunction(m_word)
        && m_interface->characterAt(nextCharPos) == QLatin1Char('(')) {
        QStringList functionSymbols = m_keywords.argsForFunction(m_word);
        IFunctionHintProposalModel *model =
                new KeywordsFunctionHintModel(functionSymbols);
        IAssistProposal *proposal = new FunctionHintProposal(m_startPosition, model);
        return proposal;
    } else {
        QList<AssistProposalItemInterface *> items;
        addWordsToProposalList(&items, m_keywords.variables(), m_variableIcon);
        addWordsToProposalList(&items, m_keywords.functions(), m_functionIcon);
        return new GenericProposal(m_startPosition, items);
    }
}

QChar KeywordsCompletionAssistProcessor::startOfCommentChar() const
{
    return QLatin1Char('#');
}

void KeywordsCompletionAssistProcessor::setKeywords(Keywords keywords)
{
    m_keywords = keywords;
}

bool KeywordsCompletionAssistProcessor::acceptsIdleEditor()
{
    const int pos = m_interface->position();
    QChar characterUnderCursor = m_interface->characterAt(pos);
    if (!characterUnderCursor.isLetterOrNumber()) {
        m_startPosition = findStartOfName();
        if (pos - m_startPosition >= 3 && !isInComment())
            return true;
    }
    return false;
}

int KeywordsCompletionAssistProcessor::findStartOfName(int pos)
{
    if (pos == -1)
        pos = m_interface->position();

    QChar chr = m_interface->characterAt(pos-1);
    if (chr == QLatin1Char('('))
        --pos;
    // Skip to the start of a name
    do {
        chr = m_interface->characterAt(--pos);
    } while (chr.isLetterOrNumber() || chr == QLatin1Char('_'));

    int start = ++pos;
    m_word.clear();
    do {
        m_word += m_interface->characterAt(pos);
        chr = m_interface->characterAt(++pos);
    } while ((chr.isLetterOrNumber() || chr == QLatin1Char('_'))
             && chr != QLatin1Char('('));

    return start;
}

bool KeywordsCompletionAssistProcessor::isInComment() const
{
    QTextCursor tc(m_interface->textDocument());
    tc.setPosition(m_interface->position());
    tc.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
    const QString &lineBeginning = tc.selectedText();
    if (lineBeginning.contains(startOfCommentChar()))
        return true;
    return false;
}

void KeywordsCompletionAssistProcessor::addWordsToProposalList(QList<AssistProposalItemInterface *> *items, const QStringList &words, const QIcon &icon)
{
    if (!items)
        return;

    for (int i = 0; i < words.count(); ++i) {
        AssistProposalItem *item = new KeywordsAssistProposalItem(m_keywords.isFunction(words.at(i)));
        item->setText(words.at(i));
        item->setIcon(icon);
        items->append(item);
    }
}

} // namespace TextEditor
