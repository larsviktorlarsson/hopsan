/*-----------------------------------------------------------------------------

 Copyright 2017 Hopsan Group

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

 The full license is available in the file GPLv3.
 For details about the 'Hopsan Group' or information about Authors and
 Contributors see the HOPSANGROUP and AUTHORS files that are located in
 the Hopsan source code root directory.

-----------------------------------------------------------------------------*/

#include <QPainter>
#include <QPaintEvent>
#include <QTextBlock>
#include <QTextDocumentFragment>
#include <QDebug>
#include <QMimeData>
#include <QMenu>
#include <QApplication>
#include <QDesktopWidget>
#include <QAbstractItemView>
#include <QStringListModel>
#include <QScrollBar>
#include <math.h>

#include "TextEditor.h"

using namespace std;

//! @brief Constructor for text editor that handles line numbers and replaces tabs with spaces
TextEditor::TextEditor(QWidget *parent) : QPlainTextEdit(parent)
{
    mpLineNumberArea = new LineNumberArea(this);

    connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(updateLineNumberAreaWidth(int)));
    connect(this, SIGNAL(updateRequest(QRect,int)), this, SLOT(updateLineNumberArea(QRect,int)));
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(highlightCurrentLine()));

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();

    setLineWrapMode(QPlainTextEdit::NoWrap);


    mpCompleter = new QCompleter(this);
    updateAutoCompleteList();

    mpCompleter->setWidget(this);
    mpCompleter->setCompletionMode(QCompleter::PopupCompletion);
    mpCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    QObject::connect(mpCompleter, SIGNAL(activated(QString)),
                     this, SLOT(insertCompletion(QString)));
}


//! @brief Computes and returns the width of the line number area
int TextEditor::lineNumberAreaWidth()
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10)
    {
        max /= 10;
        ++digits;
    }

    int space = 3 + fontMetrics().width(QLatin1Char('9')) * digits;

    return space;
}


//! @brief Updates width of line number area
void TextEditor::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}


//! @brief Update function for line number area
void TextEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        mpLineNumberArea->scroll(0, dy);
    else
        mpLineNumberArea->update(0, rect.y(), mpLineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void TextEditor::insertCompletion(const QString& completion)
{
    if (mpCompleter->widget() != this)
        return;
    QTextCursor tc = textCursor();
    int extra = completion.length() - mpCompleter->completionPrefix().length();
    tc.movePosition(QTextCursor::Left);
    tc.movePosition(QTextCursor::EndOfWord);
    tc.insertText(completion.right(extra));
    setTextCursor(tc);
}


//! @brief Resize event for text editor
void TextEditor::resizeEvent(QResizeEvent *pEvent)
{
    QPlainTextEdit::resizeEvent(pEvent);

    QRect cr = contentsRect();
    mpLineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}


//! @brief Intercepts paste operations, to control what is pastable
void TextEditor::insertFromMimeData(const QMimeData *pData)
{
    if(pData->hasText())
    {
        insertPlainText(pData->text());
    }
}


//! @brief Intercept key press event, to handle special keys
void TextEditor::keyPressEvent(QKeyEvent *pEvent)
{
    if (mpCompleter->popup()->isVisible())
    {
        //Ignore the following keys when the completer is visible
       switch (pEvent->key()) {
       case Qt::Key_Enter:
       case Qt::Key_Return:
       case Qt::Key_Escape:
       case Qt::Key_Tab:
       case Qt::Key_Backtab:
            pEvent->ignore();
            return;
       default:
           break;
       }
    }

    //Replace tabs with four whitespaces
    if(pEvent->key() == Qt::Key_Backtab)    //Shift+tab, remove spaces to the left until next tab stop (or as many spaces as possible)
    {
        QString line = this->textCursor().block().text().toLatin1();
        int pos = textCursor().positionInBlock();
        while(pos > 0 && line.at(pos-1) == ' ')
        {
            this->textCursor().deletePreviousChar();
            line = this->textCursor().block().text().toLatin1();
            pos = textCursor().positionInBlock();
            if(pos%4 == 0)
            {
                break;
            }
        }
    }
    else if(pEvent->key() == Qt::Key_Enter || pEvent->key() == Qt::Key_Return)
    {
        QString line = this->textCursor().block().text().toLatin1();
        QString newStr = "\n";
        while(line.startsWith(" "))
        {
            line.remove(0,1);
            newStr.append(" "); //Append one space to new line for each space on previous line, to keep indentation
        }
        if(line.startsWith("{"))
        {
            newStr.append("    ");  //Add an extra tab if previous line is a left bracket
        }
        insertPlainText(newStr);
    }
    else if(pEvent->key() == Qt::Key_Home)
    {
        if((pEvent->modifiers().testFlag(Qt::ShiftModifier)))
        {
            moveCursor(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor); //Shift+home = select until start of line (Excluding whitespace at beginning)
            moveCursor(QTextCursor::NextWord, QTextCursor::KeepAnchor);
        }
        else
        {
            moveCursor(QTextCursor::StartOfBlock);      //Home = move cursor to start of line, excluding initial whitespace
            moveCursor(QTextCursor::NextWord);
        }
    }
    else if(pEvent->key() == Qt::Key_Tab)
    {
        if(textCursor().anchor() != textCursor().position())    //Selection exists, indent whole selection
        {
            QString text = textCursor().selection().toPlainText();
            text.replace("\n", "\n    ");
            text.prepend("    ");
            textCursor().beginEditBlock();
            textCursor().removeSelectedText();
            insertPlainText(text);
            textCursor().endEditBlock();
        }
        else
        {
            QString line = this->textCursor().block().text().toLatin1();
            int pos = textCursor().positionInBlock();
            qDebug() << line << ", position: " << pos;
            int nSpaces = 0;
            while(line.startsWith(" "))
            {
                nSpaces++;
                line.remove(0,1);
            }
            int nSpacesToInsert = 4;    //Insert four spaces by default
            if(pos < nSpaces)           //If at beginning of line, insert until next tab stop
            {
                while((nSpaces+nSpacesToInsert)%4 != 0)
                {
                    nSpacesToInsert--;
                }
            }
            for(int i=0; i<nSpacesToInsert; ++i)
            {
                this->insertPlainText(" ");
            }
        }
    }
    else if(pEvent->key() == Qt::Key_T && pEvent->modifiers().testFlag(Qt::ControlModifier))     //Ctrl-T = block comment
    {
        textCursor().beginEditBlock();
        QString text = textCursor().selection().toPlainText();
        text.replace("\n", "\n//");
        if(textCursor().atBlockStart())
        {
            text.prepend("//");
        }
        else
        {
            int pos = textCursor().position();
            int stopPos = textCursor().anchor();
            QTextCursor c;
            c = textCursor();
            c.setPosition(min(pos,stopPos));
            setTextCursor(c);
            moveCursor(QTextCursor::StartOfBlock);
            insertPlainText("//");

            c = textCursor();
            c.setPosition(pos+2);
            c.setPosition(stopPos+2, QTextCursor::KeepAnchor);
            setTextCursor(c);
        }
        textCursor().removeSelectedText();
        insertPlainText(text);
        textCursor().endEditBlock();
    }
    else if(pEvent->key() == Qt::Key_U && pEvent->modifiers().testFlag(Qt::ControlModifier))     //Ctrl-U = uncomment block
    {
        textCursor().beginEditBlock();
        QString text = textCursor().selection().toPlainText();
        text.replace("\n//", "\n");

        int pos = textCursor().position();
        int stopPos = textCursor().anchor();
        QTextCursor c;
        c = textCursor();
        c.setPosition(min(pos,stopPos));
        setTextCursor(c);
        moveCursor(QTextCursor::StartOfBlock);
        moveCursor(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
        moveCursor(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
        int movement = 0;
        if(textCursor().selection().toPlainText() == "//")
        {
            textCursor().removeSelectedText();
            movement = 2;
        }
        c = textCursor();
        c.setPosition(pos-movement);
        c.setPosition(stopPos-movement, QTextCursor::KeepAnchor);
        setTextCursor(c);

        textCursor().removeSelectedText();
        insertPlainText(text);

        textCursor().endEditBlock();
    }
    else
    {
        QPlainTextEdit::keyPressEvent(pEvent);

        updateAutoCompleteList();

        QTextCursor tc = textCursor();
        tc.select(QTextCursor::WordUnderCursor);
        QString prefix = tc.selectedText();
        if((pEvent->key() != Qt::Key_Space || !pEvent->modifiers().testFlag(Qt::ControlModifier)) && prefix.isEmpty())
            return;

        if (prefix != mpCompleter->completionPrefix()) {
            mpCompleter->setCompletionPrefix(prefix);
            mpCompleter->popup()->setCurrentIndex(mpCompleter->completionModel()->index(0, 0));
        }
        QRect cr = cursorRect();
        cr.setWidth(mpCompleter->popup()->sizeHintForColumn(0)
                    + mpCompleter->popup()->verticalScrollBar()->sizeHint().width());
        mpCompleter->complete(cr);
    }
}

void TextEditor::wheelEvent(QWheelEvent* event)
{
#if QT_VERSION >= 0x050000  //zoomIn() and zoomOut() not available in Qt4
   if ((event->modifiers() == Qt::ControlModifier) && (event->delta() > 0))
       zoomIn(2);
   else if ((event->modifiers() == Qt::ControlModifier) && (event->delta() < 0))
       zoomOut(2);
   else
       QPlainTextEdit::wheelEvent(event);
#else
   QPlainTextEdit::wheelEvent(event);
#endif
}



//! @brief Highlights current line
void TextEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;

    if (!isReadOnly())
    {
        QTextEdit::ExtraSelection selection;

        QColor lineColor = QColor(Qt::yellow).lighter(160);

        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    setExtraSelections(extraSelections);
}


//! @brief Paint event for line number area
void TextEditor::lineNumberAreaPaintEvent(QPaintEvent *pEvent)
{
    QPainter painter(mpLineNumberArea);
    painter.fillRect(pEvent->rect(), QColor(240,240,240)/*Qt::lightGray*/);

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = int(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + int(blockBoundingRect(block).height());

    while (block.isValid() && top <= pEvent->rect().bottom())
    {
        if (block.isVisible() && bottom >= pEvent->rect().top())
        {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(Qt::black);
            painter.drawText(0, top, mpLineNumberArea->width(), fontMetrics().height(), Qt::AlignRight, number);
        }

        block = block.next();
        top = bottom;
        bottom = top + (int) blockBoundingRect(block).height();
        ++blockNumber;
    }
}


void TextEditor::updateAutoCompleteList()
{
    QStringList lines = toPlainText().split("\n");

    QStringList dataTypes = QStringList() << "double" << "int" << "SecondOrderTransferFunction" << "FirstOrderTransferFunction" << "Port";
    QStringList functions = QStringList() << "addInputVariable" << "addOutputVariable" << "addConstant" << "addPowerPort" << "getSafeNodeDataPtr";

    int bracketCounter=-1;

    QStringList variables;
    foreach(const QString &line, lines)
    {
        if(line.simplified().startsWith("class "))
            bracketCounter = 0;

        bracketCounter += line.count("{");
        bracketCounter -= line.count("}");
        if(bracketCounter != 1)
            continue;

        if(line.contains("()")) //Ignore functions
            continue;

        if(dataTypes.contains(line.simplified().section(" ",0,0)))
        {
            variables.append(line.simplified().split(","));
        }
    }
    for(int v=0; v<variables.size(); ++v)
    {
        variables[v].remove("*");
        variables[v].remove(";");
        for(int d=0; d<dataTypes.size(); ++d)
        {
            variables[v].remove(dataTypes[d]+" ");
        }
        variables[v].remove(" ");
        while(variables[v].contains("["))
        {
            variables[v].remove("["+variables[v].section("[",1,1).section("]",0,0)+"]");
        }
    }

    QStringList allWords = QStringList() << dataTypes << functions << variables;
    mpCompleter->setModel(new QStringListModel(allWords, mpCompleter));
}


//void TextEditor::updateLineNumberArea(const QRect &rect, int dy)
//{
//    if (dy)
//    {
//        mpLineNumberArea->scroll(0, dy);
//    } else if (m_countCache.first != blockCount() || m_countCache.second != textCursor().block().lineCount())
//    {
//        mpLineNumberArea->update(0, rect.y(), mpLineNumberArea->width(), rect.height());
//        m_countCache.first = blockCount();
//        m_countCache.second = textCursor().block().lineCount();
//    }

//    if (rect.contains(viewport()->rect()))
//        updateLineNumberAreaWidth(0);
//}
