/************************************************************************
**
**  Copyright (C) 2019-2025 Kevin B. Hendricks, Stratford, Ontario, Canada
**
**  This file is part of Sigil.
**
**  Sigil is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  Sigil is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with Sigil.  If not, see <http://www.gnu.org/licenses/>.
**
*************************************************************************/

#include <QString>
#include <QStringList>
#include <QHash>
#include <QFileInfo>
#include <QFile>
#include <QMimeDatabase>
#include <QMimeType>

#include "sigil_constants.h"
#include "Misc/Utility.h"
#include "Parsers/QuickParser.h"
#include "Misc/MediaTypes.h"

const QStringList IMAGE_EXTENSIONS     = QStringList() << "jpg"   << "jpeg" << "png" << "gif"  << "tif"  << "tiff"  << "bm"
                                                                  << "bmp" << "webp";
const QStringList SVG_EXTENSIONS       = QStringList() << "svg";
const QStringList SMIL_EXTENSIONS      = QStringList() << "smil";
const QStringList JPG_EXTENSIONS       = QStringList() << "jpg"   << "jpeg";
const QStringList TIFF_EXTENSIONS      = QStringList() << "tif"   << "tiff";
const QStringList MISC_TEXT_EXTENSIONS = QStringList() << "txt"   << "js";
const QStringList MISC_XML_EXTENSIONS  = QStringList() << "smil"  << "xpgt" << "pls" "xml";
const QStringList FONT_EXTENSIONS      = QStringList() << "ttf"   << "ttc"  << "otf" << "woff" << "woff2";
const QStringList TEXT_EXTENSIONS      = QStringList() << "xhtml" << "html" << "htm";
const QStringList STYLE_EXTENSIONS     = QStringList() << "css";
const QStringList AUDIO_EXTENSIONS     = QStringList() << "aac"   << "m4a"  << "mp3" << "mpeg" << "mpg" << "oga" << "ogg" << "opus";
const QStringList VIDEO_EXTENSIONS     = QStringList() << "m4v"   << "mp4"  << "mov" << "ogv"  << "webm" << "vtt" << "ttml";
const QStringList IMAGE_MIMEYPES       = QStringList() << "image/gif" << "image/jpeg" << "image/png" << "image/webp" << "image/tiff";
const QStringList SVG_MIMETYPES        = QStringList() << "image/svg+xml";
const QStringList TEXT_MIMETYPES       = QStringList() << "application/xhtml+xml" << "application/x-dtbook+xml";
const QStringList STYLE_MIMETYPES      = QStringList() << "text/css";
const QStringList FONT_MIMETYPES       = QStringList() << "application/x-font-ttf" << "application/x-font-opentype" 
                                                       << "application/vnd.ms-opentype" << "application/font-woff" 
                                                       << "application/font-sfnt" << "font/woff2";
const QStringList AUDIO_MIMETYPES      = QStringList() << "audio/mpeg" << "audio/mp3" << "audio/ogg" << "audio/mp4" << "audio/opus";
const QStringList VIDEO_MIMETYPES      = QStringList() << "video/mp4" << "video/ogg" << "video/webm" 
                                                       << "text/vtt" << "application/ttml+xml";
const QStringList MISC_XML_MIMETYPES   = QStringList() << "application/oebps-page-map+xml" 
                                                       << "application/vnd.adobe-page-map+xml"
                                                       << "application/smil+xml" 
                                                       << "application/adobe-page-template+xml" 
                                                       << "application/vnd.adobe-page-template+xml" 
                                                       << "application/pls+xml"
                                                       << "application/xml"
                                                       << "text/xml";


MediaTypes *MediaTypes::m_instance = 0;

MediaTypes *MediaTypes::instance()
{
    if (m_instance == 0) {
        m_instance = new MediaTypes();
    }

    return m_instance;
}


QString MediaTypes::GetMediaTypeFromExtension(const QString &extension, const QString &fallback)
{
    return m_ExtToMType.value(extension, fallback);
}


QString MediaTypes::GetFileDataMimeType(const QString &absolute_file_path, const QString &fallback)
{
    QMimeDatabase db;
    QString mimetype = fallback;
    QFileInfo fi(absolute_file_path);
    QFile file(absolute_file_path);
    if (file.open(QIODeviceBase::ReadOnly)) {
        QMimeType mt = db.mimeTypeForFileNameAndData(fi.fileName(), &file);
        if (mt.isValid()) {
            mimetype = mt.name();
	}
        file.close();
    }
    return mimetype;
}


QString MediaTypes::GetMediaTypeFromXML(const QString& absolute_file_path, const QString &fallback)
{
    QString mimetype = fallback;
    QString xmldata = Utility::ReadUnicodeTextFile(absolute_file_path);
    QuickParser qp(xmldata);
    while(true) {
        QuickParser::MarkupInfo mi = qp.parse_next();
        if (mi.pos < 0) break;
        if (mi.text.isEmpty()) {
            if (mi.ttype == "begin" || mi.ttype == "single") {
                mimetype = "application/xml";
                QString tag = mi.tname;
                QString prefix = "";
                if (mi.tname.contains(":")) {
                    QStringList tagpieces = mi.tname.split(':');
                    tag = tagpieces.at(1);
                    prefix = tagpieces.at(0);
                }
                QStringList attvalues = mi.tattr.values();
                if (tag == "smil" && attvalues.contains("http://www.w3.org/ns/SMIL")) {
                    mimetype = "application/smil+xml";
                }
                if (tag == "template" && attvalues.contains("http://ns.adobe.com/2006/ade")) {
                    mimetype = "application/vnd.adobe-page-template+xml";
                }
                if (tag == "page-map")        mimetype = "application/vnd.adobe-page-map+xml";
                if (tag == "display_options") mimetype = "application/apple-display-options+xml";
                if (tag == "container")       mimetype = "application/oebps-container+xml";
                if (tag == "svg")             mimetype = "image/svg+xml";
                if (tag == "package")         mimetype = "application/oebps-package+xml";
                if (tag == "encryption")      mimetype = "application/oebps-encryption+xml";
                if (tag == "plist")           mimetype = "application/vnd.apple-plist+xml";
                if (tag == "onixmessage")     mimetype = "application/onix+xml";
                if (tag == "tt")              mimetype = "application/ttml+xml";
                if (tag == "ncx")             mimetype = "application/x-dtbncx+xml";
                if (tag == "lexicon")         mimetype = "application/pls+xml";
                if (tag == "html") {
                    if (attvalues.contains("http://www.w3.org/1999/xhtml")) {
                        mimetype = "application/xhtml+xml";
                    } else {
                        mimetype = "text/html";
                    }
                }
                return mimetype;
            }
        }
    }
    return mimetype;
}


QString MediaTypes::GetGroupFromMediaType(const QString &media_type, const QString &fallback)
{
    QString group = m_MTypeToGroup.value(media_type, "");
    if (group.isEmpty()) {
        if (media_type.startsWith("image/")) group = "Images";
        if (media_type.startsWith("application/font")) group = "Fonts";
        if (media_type.startsWith("application/x-font")) group = "Fonts";
        if (media_type.startsWith("font/"))  group = "Fonts";
        if (media_type.startsWith("audio/")) group = "Audio";
        if (media_type.startsWith("video/")) group = "Video";
        if (media_type.contains("adobe") && media_type.contains("template")) group = "Misc";
        if (media_type.startsWith("application/pdf")) group = "Misc";
    }
    if (group.isEmpty()) return fallback;
    return group;
}

// epub devs use wrong mediatypes just about everyplace so try to be 
// robust to unknown mediatypes if they fit known patterns
QString MediaTypes::GetResourceDescFromMediaType(const QString &media_type, const QString &fallback)
{
    QString desc = m_MTypeToRDesc.value(media_type, "");
    if (desc.isEmpty()) {
        if (media_type.startsWith("image/")) desc = "ImageResource";
        if (media_type.startsWith("application/font")) desc = "FontResource";
        if (media_type.startsWith("application/x-font")) desc = "FontResource";
        if (media_type.startsWith("font/"))  desc = "FontResource";
        if (media_type.startsWith("audio/")) desc = "AudioResource";
        if (media_type.startsWith("video/")) desc = "VideoResource";
        if (media_type.contains("adobe") && media_type.contains("template")) desc = "XMLResource";
        if (media_type.startsWith("application/pdf")) desc = "PdfResource";
    }
    if (desc.isEmpty()) return fallback;
    return desc;
}


MediaTypes::MediaTypes()
{
    SetExtToMTypeMap();
    SetMTypeToGroupMap();
    SetMTypeToRDescMap();
}

void MediaTypes::SetExtToMTypeMap()
{
    if (!m_ExtToMType.isEmpty()) {
        return;
    }
    // default to using the preferred media-types ffrom the epub 3.2 spec
    // https://www.w3.org/publishing/epub3/epub-spec.html#sec-cmt-supported
    m_ExtToMType[ "bm"    ] = "image/bmp";
    m_ExtToMType[ "bmp"   ] = "image/bmp";
    m_ExtToMType[ "css"   ] = "text/css";
    m_ExtToMType[ "epub"  ] = "application/epub+zip";
    m_ExtToMType[ "gif"   ] = "image/gif";
    m_ExtToMType[ "htm"   ] = "application/xhtml+xml";
    m_ExtToMType[ "html"  ] = "application/xhtml+xml";
    m_ExtToMType[ "jpeg"  ] = "image/jpeg";
    m_ExtToMType[ "jpg"   ] = "image/jpeg";
    m_ExtToMType[ "js"    ] = "application/javascript";
    m_ExtToMType[ "m4a"   ] = "audio/mp4";
    m_ExtToMType[ "m4v"   ] = "video/mp4";
    m_ExtToMType[ "mp3"   ] = "audio/mpeg";
    m_ExtToMType[ "mp4"   ] = "video/mp4";
    m_ExtToMType[ "ncx"   ] = "application/x-dtbncx+xml";
    m_ExtToMType[ "oga"   ] = "audio/ogg";
    m_ExtToMType[ "ogg"   ] = "audio/ogg";
    m_ExtToMType[ "ogv"   ] = "video/ogg";
    m_ExtToMType[ "opf"   ] = "application/oebps-package+xml";
    m_ExtToMType[ "opus"  ] = "audio/opus";
    m_ExtToMType[ "otf"   ] = "font/otf";
    m_ExtToMType[ "pls"   ] = "application/pls+xml";
    m_ExtToMType[ "pdf"   ] = "application/pdf";
    m_ExtToMType[ "png"   ] = "image/png";
    m_ExtToMType[ "smil"  ] = "application/smil+xml";
    m_ExtToMType[ "svg"   ] = "image/svg+xml";
    m_ExtToMType[ "tif"   ] = "image/tiff";
    m_ExtToMType[ "tiff"  ] = "image/tiff";
    m_ExtToMType[ "ttc"   ] = "font/collection";
    m_ExtToMType[ "ttf"   ] = "font/ttf";
    m_ExtToMType[ "ttml"  ] = "application/ttml+xml";
    m_ExtToMType[ "txt"   ] = "text/plain";
    m_ExtToMType[ "vtt"   ] = "text/vtt";
    m_ExtToMType[ "webm"  ] = "video/webm";
    m_ExtToMType[ "webp"  ] = "image/webp";
    m_ExtToMType[ "woff"  ] = "font/woff";
    m_ExtToMType[ "woff2" ] = "font/woff2";
    m_ExtToMType[ "xhtml" ] = "application/xhtml+xml";
    m_ExtToMType[ "xml"   ] = "application/xml";
    m_ExtToMType[ "xpgt"  ] = "application/vnd.adobe-page-template+xml";
}


void MediaTypes::SetMTypeToGroupMap()
{
    if (!m_MTypeToGroup.isEmpty()) {
        return;
    }
    m_MTypeToGroup[ "image/jpeg"                              ] = "Images";
    m_MTypeToGroup[ "image/png"                               ] = "Images";
    m_MTypeToGroup[ "image/gif"                               ] = "Images";
    m_MTypeToGroup[ "image/svg+xml"                           ] = "Images";
    m_MTypeToGroup[ "image/bmp"                               ] = "Images";
    m_MTypeToGroup[ "image/tiff"                              ] = "Images";
    m_MTypeToGroup[ "image/webp"                              ] = "Images";

    m_MTypeToGroup[ "application/xhtml+xml"                   ] = "Text";
    m_MTypeToGroup[ "application/x-dtbook+xml"                ] = "Text";
    m_MTypeToGroup[ "text/html"                               ] = "Text";

    m_MTypeToGroup[ "font/woff2"                              ] = "Fonts";
    m_MTypeToGroup[ "font/woff"                               ] = "Fonts";
    m_MTypeToGroup[ "font/otf"                                ] = "Fonts";
    m_MTypeToGroup[ "font/ttf"                                ] = "Fonts";
    m_MTypeToGroup[ "font/sfnt"                               ] = "Fonts";
    m_MTypeToGroup[ "font/collection"                         ] = "Fonts";
    m_MTypeToGroup[ "application/vnd.ms-opentype"             ] = "Fonts";

    // deprecated font mediatypes
    // See https://www.iana.org/assignments/media-types/media-types.xhtml#font
    m_MTypeToGroup[ "application/font-ttf"                    ] = "Fonts";
    m_MTypeToGroup[ "application/font-otf"                    ] = "Fonts";
    m_MTypeToGroup[ "application/font-sfnt"                   ] = "Fonts";
    m_MTypeToGroup[ "application/font-woff"                   ] = "Fonts";
    m_MTypeToGroup[ "application/font-woff2"                  ] = "Fonts";
    m_MTypeToGroup[ "application/x-truetype-font"             ] = "Fonts";
    m_MTypeToGroup[ "application/x-opentype-font"             ] = "Fonts";
    m_MTypeToGroup[ "application/x-font-ttf"                  ] = "Fonts";
    m_MTypeToGroup[ "application/x-font-otf"                  ] = "Fonts";
    m_MTypeToGroup[ "application/x-font-opentype"             ] = "Fonts";
    m_MTypeToGroup[ "application/x-font-truetype"             ] = "Fonts";
    m_MTypeToGroup[ "application/x-font-truetype-collection"  ] = "Fonts";

    m_MTypeToGroup[ "audio/mpeg"                              ] = "Audio";
    m_MTypeToGroup[ "audio/mp3"                               ] = "Audio";
    m_MTypeToGroup[ "audio/mp4"                               ] = "Audio";
    m_MTypeToGroup[ "audio/ogg"                               ] = "Audio";
    m_MTypeToGroup[ "audio/opus"                              ] = "Audio";

    m_MTypeToGroup[ "video/mp4"                               ] = "Video";
    m_MTypeToGroup[ "video/ogg"                               ] = "Video";
    m_MTypeToGroup[ "video/webm"                              ] = "Video";
    m_MTypeToGroup[ "text/vtt"                                ] = "Video";
    m_MTypeToGroup[ "application/ttml+xml"                    ] = "Video";

    m_MTypeToGroup[ "text/css"                                ] = "Styles";

    m_MTypeToGroup[ "application/x-dtbncx+xml"                ] = "ncx";

    m_MTypeToGroup[ "application/oebps-package+xml"           ] = "opf";

    m_MTypeToGroup[ "application/oebps-page-map+xml"          ] = "Misc";
    m_MTypeToGroup[ "application/vnd.adobe-page-map+xml"      ] = "Misc";

    m_MTypeToGroup[ "application/smil+xml"                    ] = "Misc";
    m_MTypeToGroup[ "application/pls+xml"                     ] = "Misc";

    m_MTypeToGroup[ "application/xml"                         ] = "Misc";
    m_MTypeToGroup[ "text/xml"                                ] = "Misc";
    
    m_MTypeToGroup[ "application/adobe-page-template+xml"     ] = "Misc";
    m_MTypeToGroup[ "application/vnd.adobe-page-template+xml" ] = "Misc";

    m_MTypeToGroup[ "application/javascript"                  ] = "Misc";
    m_MTypeToGroup[ "application/ecmascript"                  ] = "Misc";
    m_MTypeToGroup[ "application/x-javascript"                ] = "Misc";
    m_MTypeToGroup[ "text/javascript"                         ] = "Misc";

    m_MTypeToGroup[ "application/pdf"                         ] = "Misc";

    m_MTypeToGroup[ "text/plain"                              ] = "Misc";

    m_MTypeToGroup[ "vnd.apple.ibooks+xml"                    ] = "other";
}


void MediaTypes::SetMTypeToRDescMap()
{
    if (!m_MTypeToRDesc.isEmpty()) {
        return;
    }
    m_MTypeToRDesc[ "application/xhtml+xml"                   ] = "HTMLResource";
    m_MTypeToRDesc[ "application/x-dtbook+xml"                ] = "HTMLResource";  // not a core media type
    m_MTypeToRDesc[ "text/html"                               ] = "HTMLResource";  // not a core media type

    m_MTypeToRDesc[ "text/css"                                ] = "CSSResource";

    m_MTypeToRDesc[ "application/oebps-package+xml"           ] = "OPFResource";
    m_MTypeToRDesc[ "application/x-dtbncx+xml"                ] = "NCXResource";

    m_MTypeToRDesc[ "image/jpeg"                              ] = "ImageResource";
    m_MTypeToRDesc[ "image/png"                               ] = "ImageResource";
    m_MTypeToRDesc[ "image/gif"                               ] = "ImageResource";
    m_MTypeToRDesc[ "image/bmp"                               ] = "ImageResource";  // not a core media type
    m_MTypeToRDesc[ "image/tiff"                              ] = "ImageResource";  // not a core media type
    m_MTypeToRDesc[ "image/webp"                              ] = "ImageResource";  // not a core media type

    m_MTypeToRDesc[ "image/svg+xml"                           ] = "SVGResource";

    m_MTypeToRDesc[ "font/woff2"                              ] = "FontResource";
    m_MTypeToRDesc[ "font/woff"                               ] = "FontResource";
    m_MTypeToRDesc[ "font/otf"                                ] = "FontResource";
    m_MTypeToRDesc[ "font/ttf"                                ] = "FontResource";
    m_MTypeToRDesc[ "application/vnd.ms-opentype"             ] = "FontResource";
    m_MTypeToRDesc[ "font/collection"                         ] = "FontResource";  // not a core media type

    // All of these are now deprecated
    // See: https://www.iana.org/assignments/media-types/media-types.xhtml#font
    m_MTypeToRDesc[ "application/x-font-truetype-collection"  ] = "FontResource";
    m_MTypeToRDesc[ "application/x-font-ttf"                  ] = "FontResource";
    m_MTypeToRDesc[ "application/x-font-otf"                  ] = "FontResource";
    m_MTypeToRDesc[ "application/x-font-opentype"             ] = "FontResource";
    m_MTypeToRDesc[ "application/x-font-truetype"             ] = "FontResource";
    m_MTypeToRDesc[ "application/x-truetype-font"             ] = "FontResource";
    m_MTypeToRDesc[ "application/x-opentype-font"             ] = "FontResource";
    m_MTypeToRDesc[ "application/font-ttf"                    ] = "FontResource";
    m_MTypeToRDesc[ "application/font-otf"                    ] = "FontResource";
    m_MTypeToRDesc[ "application/font-sfnt"                   ] = "FontResource";
    m_MTypeToRDesc[ "application/font-woff"                   ] = "FontResource";
    m_MTypeToRDesc[ "application/font-woff2"                  ] = "FontResource";

    m_MTypeToRDesc[ "audio/mpeg"                              ] = "AudioResource";
    m_MTypeToRDesc[ "audio/mp3"                               ] = "AudioResource";
    m_MTypeToRDesc[ "audio/mp4"                               ] = "AudioResource";
    m_MTypeToRDesc[ "audio/ogg"                               ] = "AudioResource";  // epub 3.3 now a core media type
    m_MTypeToRDesc[ "audio/opus"                              ] = "AudioResource";  // epub 3.3 now a core media type

    m_MTypeToRDesc[ "video/mp4"                               ] = "VideoResource";  
    m_MTypeToRDesc[ "video/ogg"                               ] = "VideoResource";
    m_MTypeToRDesc[ "video/webm"                              ] = "VideoResource";
    m_MTypeToRDesc[ "text/vtt"                                ] = "VideoResource";
    m_MTypeToRDesc[ "application/ttml+xml"                    ] = "VideoResource";

    m_MTypeToRDesc[ "application/smil+xml"                    ] = "XMLResource";
    m_MTypeToRDesc[ "application/pls+xml"                     ] = "XMLResource";

    m_MTypeToRDesc[ "application/oebps-page-map+xml"          ] = "XMLResource";  // not a core media type
    m_MTypeToRDesc[ "application/vnd.adobe-page-map+xml"      ] = "XMLResource";  // not a core media type
    m_MTypeToRDesc[ "application/adobe-page-template+xml"     ] = "XMLResource";  // not a core media type
    m_MTypeToRDesc[ "application/vnd.adobe-page-template+xml" ] = "XMLResource";  // not a core media type

    m_MTypeToRDesc[ "application/javascript"                  ] = "MiscTextResource";
    m_MTypeToRDesc[ "application/x-javascript"                ] = "MiscTextResource";
    m_MTypeToRDesc[ "application/ecmascript"                  ] = "MiscTextResource";
    m_MTypeToRDesc[ "text/javascript"                         ] = "MiscTextResource";
    m_MTypeToRDesc[ "text/plain"                              ] = "MiscTextResource";  // not a core media type
    
    m_MTypeToRDesc[ "application/xml"                         ] = "XMLResource";  // not a core media type
    m_MTypeToRDesc[ "text/xml"                                ] = "XMLResource";  // not a core media type

    m_MTypeToRDesc[ "application/pdf"                         ] = "PdfResource";  // not a core media type

    m_MTypeToRDesc[ "vnd.apple.ibooks+xml"                    ] = "Resource";

}

