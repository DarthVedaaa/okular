/***************************************************************************
 *   Copyright (C) 2004-2005 by Enrico Ros <eros.kde@email.it>             *
 *   Copyright (C) 2004-2005 by Albert Astals Cid <tsdgeos@terra.es>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _OKULAR_DOCUMENT_H_
#define _OKULAR_DOCUMENT_H_

#include <okular/core/okular_export.h>
#include <okular/core/area.h>
#include <okular/core/global.h>

#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QVector>
#include <QtXml/QDomDocument>
#include <QtXml/QDomElement>

#include <kmimetype.h>

class KBookmark;
class KPrinter;
class KPrintDialogPage;
class KUrl;
class KActionCollection;
class QToolBox;

namespace Okular {

class Annotation;
class BookmarkManager;
class DocumentFonts;
class DocumentInfo;
class DocumentObserver;
class DocumentSynopsis;
class DocumentViewport;
class EmbeddedFile;
class ExportFormat;
class Generator;
class Link;
class Page;
class PixmapRequest;
class SourceReference;
class VisiblePageRect;

/** IDs for seaches. Globally defined here. **/
#define PART_SEARCH_ID 1
#define PAGEVIEW_SEARCH_ID 2
#define SW_SEARCH_ID 3


/**
 * @short The Document. Heart of everything. Actions take place here.
 *
 * The Document is the main object in Okular. All views query the Document to
 * get data/properties or even for accessing pages (in a 'const' way).
 *
 * It is designed to keep it detached from the document type (pdf, ps, you
 * name it..) so whenever you want to get some data, it asks its internals
 * generator to do the job and return results in a format-indepedent way.
 *
 * Apart from the generator (the currently running one) the document stores
 * all the Pages ('Page' class) of the current document in a vector and
 * notifies all the registered DocumentObservers when some content changes.
 *
 * For a better understanding of hieracies @see README.internals.png
 * @see DocumentObserver, Page
 */
class OKULAR_EXPORT Document : public QObject
{
    Q_OBJECT

    public:
        /**
         * Creates a new document with the given list of @p generators.
         */
        Document( QHash<QString, Generator*> *generators );

        /**
         * Destroys the document.
         */
        ~Document();

        /**
         * Opens the document.
         */
        bool openDocument( const QString & docFile, const KUrl & url, const KMimeType::Ptr &mime );

        /**
         * Closes the document.
         */
        void closeDocument();

        /**
         * Registers a new @p observer for the document.
         */
        void addObserver( DocumentObserver *observer );

        /**
         * Unregisters the given @p observer for the document.
         */
        void removeObserver( DocumentObserver *observer );

        /**
         * Reparses and applies the configuration.
         */
        void reparseConfig();

        /**
         * Returns whether the document is currently opened.
         */
        bool isOpened() const;

        /**
         * Returns the meta data of the document or 0 if no meta data
         * are available.
         */
        const DocumentInfo * documentInfo() const;

        /**
         * Returns the table of content of the document or 0 if no
         * table of content is available.
         */
        const DocumentSynopsis * documentSynopsis() const;

        /**
         * Returns the description of used fonts or 0 if no description
         * is available.
         */
        const DocumentFonts * documentFonts() const;

        /**
         * Returns the list of embedded files or 0 if no embedded files
         * are available.
         */
        const QList<EmbeddedFile*> *embeddedFiles() const;

        /**
         * Returns the page object for the given page @p number or 0
         * if the number is out of range.
         */
        const Page * page( int number ) const;

        /**
         * Returns the current viewport of the document.
         */
        const DocumentViewport & viewport() const;

        /**
         * Sets the list of visible page rectangles.
         * @see VisiblePageRect
         */
        void setVisiblePageRects( const QVector< VisiblePageRect * > & visiblePageRects, int excludeId = -1 );

        /**
         * Returns the list of visible page rectangles.
         */
        const QVector< VisiblePageRect * > & visiblePageRects() const;

        /**
         * Returns the number of the current page.
         */
        uint currentPage() const;

        /**
         * Returns the number of pages of the document.
         */
        uint pages() const;

        /**
         * Returns the url of the currently opened document.
         */
        KUrl currentDocument() const;

        /**
         * Returns whether the given @p actions are allowed
         * in the document.
         * @see Permissions
         */
        bool isAllowed( Permissions actions ) const;

        /**
         * Returns whether the document supports searching.
         */
        bool supportsSearching() const;

        /**
         * Returns whether the document supports the listing of paper sizes.
         */
        bool supportsPaperSizes() const;

        /**
         * Returns the list of supported paper sizes or an empty list if this
         * feature is not available.
         * @see supportsPaperSizes
         */
        QStringList paperSizes() const;

        /**
         * Returns whether the document supports the export to ASCII text.
         */
        bool canExportToText() const;

        /**
         * Exports the document as ASCII text and saves it under @p fileName.
         */
        bool exportToText( const QString& fileName ) const;

        /**
         * Returns the list of supported export formats.
         * @see ExportFormat
         */
        QList<ExportFormat> exportFormats() const;

        /**
         * Exports the document in the given @p format and saves it under @p fileName.
         */
        bool exportTo( const QString& fileName, const ExportFormat& format ) const;

        /**
         * Returns whether the document history is at the begin.
         */
        bool historyAtBegin() const;

        /**
         * Returns whether the document history is at the end.
         */
        bool historyAtEnd() const;

        /**
         * Returns the meta data for the given @p key and @p option or an empty variant
         * if the key doesn't exists.
         */
        QVariant metaData( const QString & key, const QVariant & option = QVariant() ) const;

        /**
         * Returns the current rotation of the document.
         */
        Rotation rotation() const;

        /**
         * If all pages have the same size this method returns it, if the page sizes
         * differ an empty size object is returned.
         */
        QSizeF allPagesSize() const;

        /**
         * Returns the size string for the given @p page or an empty string
         * if the page is out of range.
         */
        QString pageSizeString( int page ) const;

        /**
         * Returns the gui description file of the document that shall be merged with
         * Okulars menu/tool bar.
         */
        QString xmlFile();

        /**
         * Calling this method allows the document to add custom actions to the gui
         * and extend Okulars side pane via @p toolbox.
         */
        void setupGui( KActionCollection *collection, QToolBox *toolbox );

        /**
         * Sets the current document viewport to the given @p page.
         *
         * @param excludeId The observer ids which shouldn't be effected by this change.
         * @param smoothMove Whether the move shall be animated smoothly.
         */
        void setViewportPage( int page, int excludeId = -1, bool smoothMove = false );

        /**
         * Sets the current document viewport to the given @p viewport.
         *
         * @param excludeId The observer ids which shouldn't be effected by this change.
         * @param smoothMove Whether the move shall be animated smoothly.
         */
        void setViewport( const DocumentViewport &viewport, int excludeId = -1, bool smoothMove = false );

        /**
         * Sets the current document viewport to the next viewport in the
         * viewport history.
         */
        void setPrevViewport();

        /**
         * Sets the current document viewport to the previous viewport in the
         * viewport history.
         */
        void setNextViewport();

        /**
         * Sets the next @p viewport in the viewport history.
         */
        void setNextDocumentViewport( const DocumentViewport &viewport );

        /**
         * Sends @p requests for pixmap generation.
         */
        void requestPixmaps( const QLinkedList<PixmapRequest*> &requests );

        /**
         * Sends a request for text page generation for the given page @p number.
         */
        void requestTextPage( uint number );

        /**
         * Adds a new @p annotation to the given @p page.
         */
        void addPageAnnotation( int page, Annotation *annotation );

        /**
         * Modifies the given @p annotation on the given @p page.
         */
        void modifyPageAnnotation( int page, Annotation *annotation );

        /**
         * Removes the given @p annotation from the given @p page.
         */
        void removePageAnnotation( int page, Annotation *annotation );

        /**
         * Removes the given @p annotations from the given @p page.
         */
        void removePageAnnotations( int page, QList<Annotation*> annotations );

        /**
         * Sets the text selection for the given @p page.
         *
         * @param rect The rectangle of the selection.
         * @param color The color of the selection.
         */
        void setPageTextSelection( int page, RegularAreaRect * rect, const QColor & color );

        /**
         * Describes the possible search types.
         */
        enum SearchType
        {
            NextMatch,      ///< Search next match
            PreviousMatch,  ///< Search previous match
            AllDocument,    ///< Search complete document
            GoogleAll,      ///< Search all words in google style
            GoogleAny       ///< Search any words in google style
        };

        /**
         * Searches the given @p text in the document.
         *
         * @param searchID The unique id for this search request.
         * @param fromStart Whether the search should be started at begin of the document.
         * @param caseSensitivity Whether the search is case sensitive.
         * @param type The type of the search. @see SearchType
         * @param moveViewport Whether the viewport shall be moved to the position of the matches.
         * @param color The highlighting color of the matches.
         * @param noDialogs Whether a search dialog shall be shown.
         */
        bool searchText( int searchID, const QString & text, bool fromStart, Qt::CaseSensitivity caseSensitivity,
                         SearchType type, bool moveViewport, const QColor & color, bool noDialogs = false );

        /**
         * Continues the search for the given @p searchID.
         */
        bool continueSearch( int searchID );

        /**
         * Resets the search for the given @p searchID.
         */
        void resetSearch( int searchID );

        /**
         * Continues with the last search.
         */
        bool continueLastSearch();

        /**
         * Adds a bookmark for the given @p page.
         */
        void addBookmark( int page );

        /**
         * Adds a bookmark for the given @p referUrl at @p viewport.
         *
         * @param title The title of the bookmark.
         */
        void addBookmark( const KUrl& referUrl, const Okular::DocumentViewport& viewport, const QString& title = QString() );

        /**
         * Returns whether the given @p page is bookmarked.
         */
        bool isBookmarked( int page ) const;

        /**
         * Removes the given @p bookmark with the given @p referUrl.
         */
        void removeBookmark( const KUrl& referUrl, const KBookmark& bookmark );

        /**
         * Returns the bookmark manager of the document.
         */
        const BookmarkManager * bookmarkManager() const;

        /**
         * Processes/Executes the given @p link.
         */
        void processLink( const Link *link );

        /**
         * Processes/Executes the given source @p reference.
         */
        void processSourceReference( const SourceReference *reference );

        /**
         * Returns whether the document can configure the printer itself.
         */
        bool canConfigurePrinter() const;

        /**
         * Prints the document to the given @p printer.
         */
        bool print( KPrinter &printer );

        /**
         * Returns a custom printer configuration page or 0 if no
         * custom printer configuration page is available.
         */
        KPrintDialogPage* configurationWidget() const;

        /**
         * This method is used by the generators to signal the finish of
         * the pixmap generation @p request.
         */
        void requestDone( PixmapRequest * request );

    public Q_SLOTS:
        /**
         * This slot is called whenever the user changes the @p rotation of
         * the document.
         */
        void slotRotation( int rotation );

        /**
         * This slot is called whenever the user changes the paper @p size
         * of the document.
         */
        void slotPaperSizes( int size );

    Q_SIGNALS:
        /**
         * This signal is emitted whenever a link action requests a
         * document close operation.
         */
        void close();

        /**
         * This signal is emitted whenever a link action requests an
         * application quit operation.
         */
        void quit();

        /**
         * This signal is emitted whenever a link action requests a
         * find operation.
         */
        void linkFind();

        /**
         * This signal is emitted whenever a link action requests a
         * goto operation.
         */
        void linkGoToPage();

        /**
         * This signal is emitted whenever a link action requests a
         * start presentation operation.
         */
        void linkPresentation();

        /**
         * This signal is emitted whenever a link action requests an
         * end presentation operation.
         */
        void linkEndPresentation();

        /**
         * This signal is emitted whenever a link action requests an
         * open url operation for the given document @p url.
         */
        void openUrl( const KUrl &url );

        /**
         * This signal is emitted whenever an error occurred.
         *
         * @param text The description of the error.
         * @param duration The time in seconds the message should be shown to the user.
         */
        void error( const QString &text, int duration );

        /**
         * This signal is emitted to signal a warning.
         *
         * @param text The description of the warning.
         * @param duration The time in seconds the message should be shown to the user.
         */
        void warning( const QString &text, int duration );

        /**
         * This signal is emitted to signal a notice.
         *
         * @param text The description of the notice.
         * @param duration The time in seconds the message should be shown to the user.
         */
        void notice( const QString &text, int duration );

    private:
        class Private;
        Private* const d;

        Q_DISABLE_COPY( Document )

        Q_PRIVATE_SLOT( d, void saveDocumentInfo() const )
        Q_PRIVATE_SLOT( d, void slotTimedMemoryCheck() )
        Q_PRIVATE_SLOT( d, void sendGeneratorRequest() )
        Q_PRIVATE_SLOT( d, void rotationFinished( int page ) )
};


/**
 * @short A view on the document.
 *
 * The Viewport structure is the 'current view' over the document. Contained
 * data is broadcasted between observers to synchronize their viewports to get
 * the 'I scroll one view and others scroll too' views.
 */
class OKULAR_EXPORT DocumentViewport
{
    public:
        /**
         * Creates a new viewport for the given page @p number.
         */
        DocumentViewport( int number = -1 );

        /**
         * Creates a new viewport from the given xml @p description.
         */
        DocumentViewport( const QString &description );

        /**
         * Returns the viewport as xml description.
         */
        QString toString() const;

        /**
         * Returns whether the viewport is valid.
         */
        bool isValid() const;

        /**
         * @internal
         */
        bool operator==( const DocumentViewport &other ) const;

        /**
         * The number of the page nearest the center of the viewport.
         */
        int pageNumber;

        /**
         * Describes the relative position of the viewport.
         */
        enum Position
        {
            Center = 1,  ///< Relative to the center of the page.
            TopLeft = 2  ///< Relative to the top left corner of the page.
        };

        /**
         * If 'rePos.enabled == true' then this structure contains the
         * viewport center.
         */
        struct {
            bool enabled;
            double normalizedX;
            double normalizedY;
            Position pos;
        } rePos;

        /**
         * If 'autoFit.enabled == true' then the page must be autofitted in the viewport.
         */
        struct {
            bool enabled;
            bool width;
            bool height;
        } autoFit;
};

/**
 * @short A DOM tree containing information about the document.
 *
 * The DocumentInfo structure can be filled in by generators to display
 * metadata about the currently opened file.
 */
class OKULAR_EXPORT DocumentInfo : public QDomDocument
{
    public:
        /**
         * Creates a new document info.
         */
        DocumentInfo();

        /**
         * Sets a value for a special key. The title should be an i18n'ed
         * string, since it's used in the document information dialog.
         */
        void set( const QString &key, const QString &value,
                  const QString &title = QString() );

        /**
         * Returns the value for a given key or an empty string when the
         * key doesn't exist.
         */
        QString get( const QString &key ) const;
};

/**
 * @short A DOM tree that describes the Table of Contents.
 *
 * The Synopsis (TOC or Table Of Contents for friends) is represented via
 * a dom tree where each node has an internal name (displayed in the listview)
 * and one or more attributes.
 *
 * In the tree the tag name is the 'screen' name of the entry. A tag can have
 * attributes. Here follows the list of tag attributes with meaning:
 * - Icon: An icon to be set in the Lisview for the node
 * - Viewport: A string description of the referred viewport
 * - ViewportName: A 'named reference' to the viewport that must be converted
 *      using metaData( "NamedViewport", *viewport_name* )
 */
class OKULAR_EXPORT DocumentSynopsis : public QDomDocument
{
    public:
        /**
         * Creates a new document synopsis object.
         */
        DocumentSynopsis();

        /**
         * Creates a new document synopsis object with the given
         * @p document as parent node.
         */
        DocumentSynopsis( const QDomDocument &document );
};

/**
 * @short A DOM thee describing fonts used in document.
 *
 * Root's childrend (if any) are font nodes with the following attributes:
 * - Name
 * - Type
 * - Embedded (if font is shipped inside the document)
 * - File (system's file that provides this font
 */
class OKULAR_EXPORT DocumentFonts : public QDomDocument
{
    public:
        /**
         * Creates a new document fonts object.
         */
        DocumentFonts();
};

/**
 * @short An embedded file into the document.
 *
 * This class represents a sort of interface of an embedded file in a document.
 *
 * Generators \b must re-implement its members to give the all the information
 * about an embedded file, like its name, its description, the date of creation
 * and modification, and the real data of the file.
 */
class OKULAR_EXPORT EmbeddedFile
{
    public:
        /**
         * Creates a new embedded file.
         */
        EmbeddedFile();

        /**
         * Destroys the embedded file.
         */
        virtual ~EmbeddedFile();

        /**
         * Returns the name of the file
         */
        virtual QString name() const = 0;

        /**
         * Returns the description of the file, or an empty string if not
         * available
         */
        virtual QString description() const = 0;

        /**
         * Returns the real data representing the file contents
         */
        virtual QByteArray data() const = 0;

        /**
         * Returns the modification date of the file, or an invalid date
         * if not available
         */
        virtual QDateTime modificationDate() const = 0;

        /**
         * Returns the creation date of the file, or an invalid date
         * if not available
         */
        virtual QDateTime creationDate() const = 0;
};

/**
 * @short An area of a specified page
 */
class OKULAR_EXPORT VisiblePageRect
{
    public:
        /**
         * Creates a new visible page rectangle.
         *
         * @param pageNumber The page number where the rectangle is located.
         * @param rectangle The rectangle in normalized coordinates.
         */
        VisiblePageRect( int pageNumber = -1, const NormalizedRect &rectangle = NormalizedRect() );

        /**
         * The page number where the rectangle is located.
         */
        int pageNumber;

        /**
         * The rectangle in normalized coordinates.
         */
        NormalizedRect rect;
};

}

#endif
