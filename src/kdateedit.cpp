/*
  This file is part of libkdepim.

  Copyright (c) 2002 Cornelius Schumacher <schumacher@kde.org>
  Copyright (c) 2002 David Jarvie <software@astrojar.org.uk>
  Copyright (c) 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  Copyright (c) 2004 Tobias Koenig <tokoe@kde.org>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

//krazy:excludeall=qclasses as we want to subclass from QComboBox, not KComboBox

#include "kdateedit.h"

#include <QDesktopWidget>

#include <klocalizedstring.h>

#include <QAbstractItemView>
#include <QApplication>
#include <QEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include <QValidator>

class DateValidator : public QValidator
{
  public:
    DateValidator( const QStringList &keywords, QWidget *parent )
      : QValidator( parent ), mKeywords( keywords )
    {}

    virtual State validate( QString &str, int & ) const
    {
      int length = str.length();

      // empty string is intermediate so one can clear the edit line and start from scratch
      if ( length <= 0 ) {
        return Intermediate;
      }

      if ( mKeywords.contains( str.toLower() ) ) {
        return Acceptable;
      }

      bool ok = QLocale().toDate( str ).isValid();
      if ( ok ) {
        return Acceptable;
      } else {
        return Intermediate;
      }
    }

  private:
    QStringList mKeywords;
};

KDateEdit::KDateEdit( QWidget *parent, const char *name )
  : QComboBox( parent ), mReadOnly( false ), mDiscardNextMousePress( false )
{
  setObjectName( name );
  // need at least one entry for popup to work
  setMaxCount( 1 );
  setEditable( true );

  mDate = QDate::currentDate();
  QString today = QLocale().toString( mDate, QLocale::ShortFormat );

  addItem( today );
  setCurrentIndex( 0 );
  setSizeAdjustPolicy( AdjustToContents );

  connect( lineEdit(), SIGNAL( returnPressed() ),
           this, SLOT( lineEnterPressed() ) );
  connect( this, SIGNAL( editTextChanged( const QString& ) ),
           SLOT( slotTextChanged( const QString& ) ) );

//  mPopup = new KDatePickerPopup( KDatePickerPopup::DatePicker | KDatePickerPopup::Words,
//                                 QDate::currentDate(), this );
  mPopup = new KDatePickerPopup( KDatePickerPopup::DatePicker, QDate::currentDate(), this );
  mPopup->hide();
  mPopup->installEventFilter( this );

  connect( mPopup, SIGNAL( dateChanged( const QDate& ) ),
           SLOT( dateSelected( const QDate& ) ) );

  // handle keyword entry
  setupKeywords();
  lineEdit()->installEventFilter( this );

  setValidator( new DateValidator( mKeywordMap.keys(), this ) );

  mTextChanged = false;
}

KDateEdit::~KDateEdit()
{
}

void KDateEdit::setDate( const QDate &date )
{
  assignDate( date );
  updateView();
}

QDate KDateEdit::date() const
{
  return mDate;
}

void KDateEdit::setReadOnly( bool readOnly )
{
  mReadOnly = readOnly;
  lineEdit()->setReadOnly( readOnly );
}

bool KDateEdit::isReadOnly() const
{
  return mReadOnly;
}

void KDateEdit::showPopup()
{
  if ( mReadOnly ) {
    return;
  }

  QRect desk =  QApplication::desktop()->screenGeometry( this );

  QPoint popupPoint = mapToGlobal( QPoint( 0, 0 ) );

  int dateFrameHeight = mPopup->sizeHint().height();
  if ( popupPoint.y() + height() + dateFrameHeight > desk.bottom() ) {
    popupPoint.setY( popupPoint.y() - dateFrameHeight );
  } else {
    popupPoint.setY( popupPoint.y() + height() );
  }

  int dateFrameWidth = mPopup->sizeHint().width();
  if ( popupPoint.x() + dateFrameWidth > desk.right() ) {
    popupPoint.setX( desk.right() - dateFrameWidth );
  }

  if ( popupPoint.x() < desk.left() ) {
    popupPoint.setX( desk.left() );
  }

  if ( popupPoint.y() < desk.top() ) {
    popupPoint.setY( desk.top() );
  }

  if ( mDate.isValid() ) {
    mPopup->setDate( mDate );
  } else {
    mPopup->setDate( QDate::currentDate() );
  }

  mPopup->popup( popupPoint );

  // The combo box is now shown pressed. Make it show not pressed again
  // by causing its (invisible) list box to emit a 'selected' signal.
  // First, ensure that the list box contains the date currently displayed.
  QDate date = parseDate();
  assignDate( date );
  updateView();

  // Now, simulate an Enter to unpress it
  QAbstractItemView *lb = view();
  if ( lb ) {
    lb->setCurrentIndex( lb->model()->index( 0, 0 ) );
    QKeyEvent *keyEvent =
      new QKeyEvent( QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier );
    QApplication::postEvent( lb, keyEvent );
  }
}

void KDateEdit::dateSelected( const QDate &date )
{
  if ( assignDate( date ) ) {
    updateView();
    emit dateChanged( date );
    emit dateEntered( date );

    if ( date.isValid() ) {
      mPopup->hide();
    }
  }
}

void KDateEdit::lineEnterPressed()
{

  bool replaced = false;

  QDate date = parseDate( &replaced );

  if ( assignDate( date ) ) {
    if ( replaced ) {
      updateView();
    }

    emit dateChanged( date );
    emit dateEntered( date );
  }
}

QDate KDateEdit::parseDate( bool *replaced ) const
{
  QString text = currentText();
  QDate result;
  
  if ( replaced ) {
    (*replaced) = false;
  }

  if ( text.isEmpty() ) {
    result = QDate();
  } else if ( mKeywordMap.contains( text.toLower() ) ) {
    QDate today = QDate::currentDate();
    int i = mKeywordMap[ text.toLower() ];
    if ( i >= 100 ) {
      /* A day name has been entered. Convert to offset from today.
       * This uses some math tricks to figure out the offset in days
       * to the next date the given day of the week occurs. There
       * are two cases, that the new day is >= the current day, which means
       * the new day has not occurred yet or that the new day < the current day,
       * which means the new day is already passed (so we need to find the
       * day in the next week).
       */
      i -= 100;
      int currentDay = today.dayOfWeek();
      if ( i >= currentDay ) {
        i -= currentDay;
      } else {
        i += 7 - currentDay;
      }
    }

    result = today.addDays( i );
    if ( replaced ) {
      (*replaced) = true;
    }
  } else {
	  printf("1\n");
    result = QLocale().toDate( text, QLocale::ShortFormat );
  }

  return result;
}

void KDateEdit::focusOutEvent( QFocusEvent *e )
{
  if ( mTextChanged ) {
    lineEnterPressed();
    mTextChanged = false;
  }
  QComboBox::focusOutEvent( e );
}

void KDateEdit::keyPressEvent(QKeyEvent* e)
{
      int step = 0;
      if ( e->key() == Qt::Key_Up ) {	      
        step = 1;
      } else if ( e->key() == Qt::Key_Down ) {
        step = -1;
      } else if ( e->key() == Qt::Key_Return ) {
	emit returnPressed();
	lineEnterPressed();
      }
      if ( step && !mReadOnly ) {
        QDate date = parseDate();
        if ( date.isValid() ) {
          date = date.addDays( step );
          if ( assignDate( date ) ) {
            updateView();
            emit dateChanged( date );
            emit dateEntered( date );
          }
        }
      }
      QComboBox::keyPressEvent(e);
}

bool KDateEdit::eventFilter( QObject *object, QEvent *event )
{
  if ( object == lineEdit() ) {
    // We only process the focus out event if the text has changed
    // since we got focus
    if ( ( event->type() == QEvent::FocusOut ) && mTextChanged ) {
      lineEnterPressed();
      mTextChanged = false;
    } else if ( event->type() == QEvent::KeyPress ) {
      // Up and down arrow keys step the date
      QKeyEvent *keyEvent = (QKeyEvent *)event;
      if ( keyEvent->key() == Qt::Key_Return ) {
        lineEnterPressed();
        return true;
      }
    }
  } else {
    // It's a date picker event
    switch ( event->type() ) {
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseButtonPress:
    {
      QMouseEvent *mouseEvent = (QMouseEvent*)event;
      if ( !mPopup->rect().contains( mouseEvent->pos() ) ) {
        QPoint globalPos = mPopup->mapToGlobal( mouseEvent->pos() );
        if ( QApplication::widgetAt( globalPos ) == this ) {
          // The date picker is being closed by a click on the
          // KDateEdit widget. Avoid popping it up again immediately.
          mDiscardNextMousePress = true;
        }
      }

      break;
    }
    default:
      break;
    }
  }

  return false;
}

void KDateEdit::mousePressEvent( QMouseEvent *event )
{
  if ( event->button() == Qt::LeftButton && mDiscardNextMousePress ) {
    mDiscardNextMousePress = false;
    return;
  }

  QComboBox::mousePressEvent( event );
}

void KDateEdit::slotTextChanged( const QString & )
{

  QDate date = parseDate(); 

  if ( assignDate( date ) ) {
    if (date.isValid() ) emit dateChanged( date );
  }

  mTextChanged = true;
}

void KDateEdit::setupKeywords()
{
  // Create the keyword list. This will be used to match against when the user
  // enters information.
  mKeywordMap.insert( i18nc( "the day after today", "tomorrow" ), 1 );
  mKeywordMap.insert( i18nc( "this day", "today" ), 0 );
  mKeywordMap.insert( i18nc( "the day before today", "yesterday" ), -1 );

  QString dayName;
  for ( int i = 1; i <= 7; ++i ) {
    dayName = QDate::longDayName( i, QDate::StandaloneFormat ).toLower();
    mKeywordMap.insert( dayName, i + 100 );
  }
}

bool KDateEdit::assignDate( const QDate &date )
{
  mDate = date;
  mTextChanged = false;
  return true;
}

void KDateEdit::updateView()
{
  QString dateString;
  if ( mDate.isValid() ) {
    dateString = QLocale().toString( mDate, QLocale::ShortFormat );
  }

  // We do not want to generate a signal here,
  // since we explicitly setting the date
  bool blocked = signalsBlocked();
  blockSignals( true );
  removeItem( 0 );
  insertItem( 0, dateString );
  blockSignals( blocked );
}

#include "kdateedit.moc"
