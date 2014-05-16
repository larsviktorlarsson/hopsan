/*-----------------------------------------------------------------------------
 This source file is part of Hopsan NG

 Copyright (c) 2011 
    Mikael Axin, Robert Braun, Alessandro Dell'Amico, Björn Eriksson,
    Peter Nordin, Karl Pettersson, Petter Krus, Ingo Staack

 This file is provided "as is", with no guarantee or warranty for the
 functionality or reliability of the contents. All contents in this file is
 the original work of the copyright holders at the Division of Fluid and
 Mechatronic Systems (Flumes) at Linköping University. Modifying, using or
 redistributing any part of this file is prohibited without explicit
 permission from the copyright holders.
-----------------------------------------------------------------------------*/

//!
//! @file   GUIWidgets.h
//! @author Flumes <flumes@lists.iei.liu.se>
//! @date   2010-01-01
//!
//! @brief Contains the GUIWidgets classes
//!
//$Id$

#ifndef GUIWIDGETS_H
#define GUIWIDGETS_H

#include <QGraphicsWidget>
#include <QObject>

#include "GUIObject.h"
#include "common.h"

#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QComboBox>
#include <QToolButton>
#include <QCheckBox>
#include <QDialog>
#include <QPointer>

enum WidgetTypesEnumT {UndefinedWidgetType, TextBoxWidgetType};

class Widget : public WorkspaceObject
{
    Q_OBJECT

public:
    Widget(QPointF pos, double rot, SelectionStatusEnumT startSelected, ContainerObject *pSystem, QGraphicsItem *pParent=0);
    virtual WidgetTypesEnumT getWidgetType() const = 0;
    int getWidgetIndex();

    // Type info
    enum { Type = WidgetType };
    int type() const;

protected:
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);

public slots:
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

protected:
    int mWidgetIndex;
    bool mIsResizing;
};


class TextBoxWidget : public Widget
{
    Q_OBJECT

public:
    TextBoxWidget(QString text, QPointF pos, double rot, SelectionStatusEnumT startSelected, ContainerObject *pSystem, size_t widgetIndex, QGraphicsItem *pParent=0);
    TextBoxWidget(const TextBoxWidget &other, ContainerObject *pSystem);

    // Type info
    virtual WidgetTypesEnumT getWidgetType() const;
    virtual QString getHmfTagName() const;

    // Save and load
    void saveToDomElement(QDomElement &rDomElement);
    void loadFromDomElement(QDomElement domElement);

    void setText(QString text);
    void setFont(QFont font);
    void setTextColor(QColor color);
    void setLineWidth(int value);
    void setLineStyle(Qt::PenStyle style);
    void setLineColor(QColor color);
    void setSize(double w, double h);
    void setBoxVisible(bool boxVisible);

    void makeSureBoxNotToSmallForText();
    void resizeBoxToText();
    void resizeTextToBox();
    void setReflowText(bool doReflow);

protected:
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event);
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

    virtual void refreshSelectionBoxSize();

public slots:
    void deleteMe(UndoStatusEnumT undoSettings=Undo);
    virtual void flipVertical(UndoStatusEnumT undoSettings = Undo);
    virtual void flipHorizontal(UndoStatusEnumT undoSettings = Undo);

private slots:
    void updateWidgetFromDialog();
    void openFontDialog();
    void openTextColorDialog();
    void openLineColorDialog();

private:
    void refreshWidgetSize();

    QGraphicsTextItem *mpTextItem;
    QGraphicsRectItem *mpBorderItem;

    QPointer<QDialog> mpEditDialog;
    QTextEdit *mpDialogTextBox;
    QPushButton *mpDialogFontButton;
    QToolButton *mpDialogLineColorButton;
    QToolButton *mpDialogTextColorButton;
    QCheckBox *mpDialogReflowCheckBox;

    QCheckBox *mpDialogShowBorderCheckBox;
    QSpinBox *mpDialogLineWidth;
    QComboBox *mpDialogLineStyle;

    QColor mSelectedTextColor;
    QColor mSelectedLineColor;

    bool mReflowText;
    bool mResizeTop;
    bool mResizeBottom;
    bool mResizeLeft;
    bool mResizeRight;

    QPointF mPosBeforeResize;
    double mWidthBeforeResize;
    double mHeightBeforeResize;
};

#endif // GUIWIDGETS_H
