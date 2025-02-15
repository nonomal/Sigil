#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# vim:ts=4:sw=4:softtabstop=4:smarttab:expandtab

# Copyright (c) 2014-2023 Kevin B. Hendricks, John Schember, and Doug Massay
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this list of
# conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice, this list
# of conditions and the following disclaimer in the documentation and/or other materials
# provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
# SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
# TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
# WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import sys, os, codecs
from collections import OrderedDict
from hrefutils import urlencodepart, urldecodepart

# encode to make xml safe
def xmlencode(data):
    if data is None:
        return ''
    newdata = xmldecode(data)
    newdata = newdata.replace('&', '&amp;')
    newdata = newdata.replace('<', '&lt;')
    newdata = newdata.replace('>', '&gt;')
    newdata = newdata.replace('"', '&quot;')
    return newdata

#decode xml encoded strings
def xmldecode(data):
    if data is None:
        return ''
    newdata = data
    newdata = newdata.replace('&quot;', '"')
    newdata = newdata.replace('&gt;', '>')
    newdata = newdata.replace('&lt;', '<')
    newdata = newdata.replace('&amp;', '&')
    return newdata

WHITESPACE_CHARS = (' ', '\n', '\r', '\t')

SPECIAL_HANDLING_TAGS = OrderedDict([
    ('?xml', ('xmlheader', -1)),
    ('!--',  ('comment', -3)),
    ('!DOCTYPE', ('doctype', -1))
    ])

SPECIAL_HANDLING_TYPES = ['xmlheader', 'doctype', 'comment']

_OPF_PARENT_TAGS = ['package', 'metadata', 'dc-metadata', 'x-metadata', 'manifest', 'spine', 'tours', 'guide', 'bindings']

class Opf_Parser(object):

    def __init__(self, opfdata):
        self.opf = opfdata
        self.opos = 0
        self.package = None
        self.metadata_attr = None
        self.metadata = []
        self.manifest = []
        self.spine_attr = None
        self.spine=[]
        self.guide=[]
        self.bindings=[]
        self.ns_remap = False
        self.used_ids = {}
        self._parseData()


    # OPF tag iterator
    def _opf_tag_iter(self):
        tcontent = last_tattr = None
        prefix = []
        while True:
            text, tag = self._parseopf()
            if text is None and tag is None:
                break
            if text is not None:
                tcontent = text.rstrip(" \r\n")
            else: # we have a tag
                ttype, tname, tattr = self._parsetag(tag)
                # remap opf namespace on tags if needed
                if tname.startswith('opf:'):
                    self.ns_remap = True
                    tname = tname[4:]
                if ttype == "begin":
                    tcontent = None
                    prefix.append(tname)
                    if tname in _OPF_PARENT_TAGS:
                        yield ".".join(prefix), tname, tattr, tcontent
                    else:
                        last_tattr = tattr
                else: # single or end
                    if ttype == "end":
                        prefix.pop()
                        tattr = last_tattr
                        if tattr is None:
                            tattr = OrderedDict()
                        last_tattr = None
                    elif ttype == 'single':
                        tcontent = None
                    if ttype == 'single' or (ttype == 'end' and tname not in _OPF_PARENT_TAGS):
                        yield ".".join(prefix), tname, tattr, tcontent
                    tcontent = None

    # now parse the OPF to extract manifest, spine , and metadata
    def _parseData(self):
        cnt = 0
        for prefix, tname, tattr, tcontent in self._opf_tag_iter():
            # package
            if tname == "package":
                ver = tattr.pop("version", "2.0")
                uid = tattr.pop("unique-identifier","bookid")
                if self.ns_remap:
                    if "xmlns:opf" in tattr:
                        tattr.pop("xmlns:opf")
                        tattr["xmlns"] = "http://www.idpf.org/2007/opf"
                self.package = (ver, uid, tattr)
                continue
            # metadata
            if tname == "metadata":
                if self.ns_remap:
                    if not "xmlns:opf" in tattr:
                        tattr["xmlns:opf"] = "http://www.idpf.org/2007/opf"
                self.metadata_attr = tattr
                continue
            if tname in ["meta", "link"] or tname.startswith("dc:") and "metadata" in prefix:
                self.metadata.append((tname, tcontent, tattr))
                continue
            # manifest
            if tname == "item" and  "manifest" in prefix:
                nid = "xid%03d" %  cnt
                cnt += 1
                id = tattr.pop("id",nid)
                # must keep all hrefs in encoded) form
                # if relative, then no fragments so decode and then encode for safety
                href = tattr.pop("href","")
                if href.find(':') == -1:
                    href = urldecodepart(href)
                    href = urlencodepart(href)
                mtype = tattr.pop("media-type","")
                self.manifest.append((id, href, mtype, tattr))
                continue
            # spine
            if tname == "spine":
                self.spine_attr = tattr
                continue
            if tname == "itemref" and "spine" in prefix:
                idref = tattr.pop("idref","")
                self.spine.append((idref, tattr))
                continue
            # guide
            if tname == "reference" and  "guide" in prefix:
                type = tattr.pop("type","")
                title = tattr.pop("title","")
                # must keep all hrefs in quoted (encoded) form
                href = tattr.pop("href","")
                self.guide.append((type, title, href))
                continue
            # bindings
            if tname in ["mediaType", "mediatype"] and "bindings" in prefix:
                mtype = tattr.pop("media-type","")
                handler = tattr.pop("handler","")
                self.bindings.append((mtype, handler))
                continue

    # parse and return either leading text or the next tag
    def _parseopf(self):
        p = self.opos
        if p >= len(self.opf):
            return None, None
        if self.opf[p] != '<':
            res = self.opf.find('<',p)
            if res == -1 :
                res = len(self.opf)
            self.opos = res
            return self.opf[p:res], None
        # handle comment as a special case
        if self.opf[p:p+4] == '<!--':
            te = self.opf.find('-->',p+1)
            if te != -1:
                te = te+2
        else:
            te = self.opf.find('>',p+1)
            ntb = self.opf.find('<',p+1)
            if ntb != -1 and ntb < te:
                self.opos = ntb
                return self.opf[p:ntb], None
        self.opos = te + 1
        return None, self.opf[p:te+1]

    # parses tag to identify:  [tname, ttype, tattr]
    #    tname: tag name,    ttype: tag type ('begin', 'end' or 'single');
    #    tattr: dictionary of tag atributes
    def _parsetag(self, s):
        n = len(s)
        p = 1
        tname = None
        ttype = None
        tattr = OrderedDict()
        while p < n and s[p:p+1] == ' ' : p += 1
        if s[p:p+1] == '/':
            ttype = 'end'
            p += 1
            while p < n and s[p:p+1] == ' ' : p += 1
        b = p
        # handle comment special case as there may be no spaces to
        # delimit name begin or end
        if s[b:].startswith('!--'):
            p = b+3
            tname = '!--'
            ttype, backstep = SPECIAL_HANDLING_TAGS[tname]
            tattr['special'] = s[p:backstep].strip()
            return tname, ttype, tattr
        while p < n and s[p:p+1] not in ('>', '/', ' ', '"', "'","\r","\n") : p += 1
        tname=s[b:p].lower()
        # remove redundant opf: namespace prefixes on opf tags
        # if tname.startswith("opf:"):
        #    tname = tname[4:]
        # more special cases
        if tname == '!doctype':
            tname = '!DOCTYPE'
        if tname in SPECIAL_HANDLING_TAGS:
            ttype, backstep = SPECIAL_HANDLING_TAGS[tname]
            tattr['special'] = s[p:backstep]
        if ttype is None:
            # parse any attributes of begin or single tags
            while s.find('=',p) != -1 :
                while p < n and s[p:p+1] in WHITESPACE_CHARS : p += 1
                b = p
                while p < n and s[p:p+1] != '=' : p += 1
                aname = s[b:p]
                aname = aname.rstrip(' \n\r\t')
                p += 1
                while p < n and s[p:p+1] in WHITESPACE_CHARS: p += 1
                if s[p:p+1] in ('"', "'") :
                    qt = s[p:p+1]
                    p = p + 1
                    b = p
                    # try to work around missing end quotes
                    while p < n and s[p:p+1] not in ['>', '<', qt] : p += 1
                    val = s[b:p]
                    p += 1
                else :
                    b = p
                    while p < n and s[p:p+1] not in ('>', '/', ' ') : p += 1
                    val = s[b:p]
                tattr[aname] = val
                if aname == "id":
                    self.used_ids[val]=1
        if ttype is None:
            ttype = 'begin'
            if s.find('/',p) >= 0:
                ttype = 'single'
        return ttype, tname, tattr

    def xlate_dict(self, attr):
        kvlist = []
        if attr is not None:
            for key in list(attr.keys()):
                kvlist.append([key,attr[key]]) 
        return kvlist

    def build_dict(self, kvlist):
        attr = OrderedDict();
        for (k, v) in kvlist:
            attr[k] = v
        return attr

    def get_used_ids(self):
        return self.used_ids

    def get_package(self):
        (ver, uid, attr) = self.package
        kvlist = self.xlate_dict(attr)
        return (ver, uid, kvlist)

    def set_package(self, ver, uid, kvlist):
        attr = self.build_dict(kvlist)
        self.package = (ver, uid, attr)

    def get_metadata_attr(self):
        kvlist = self.xlate_dict(self.metadata_attr)
        return kvlist

    def set_metadata_attr(self, kvlist):
        self.metadata_attr = self.build_dict(kvlist)

    def get_metadata(self):
        metadata = []
        for (mname, mcontent, attr) in self.metadata:
            kvlist = self.xlate_dict(attr)
            metadata.append((mname, mcontent, kvlist))
        return metadata

    def set_metadata(self, metadata):
        self.metadata = []
        for (mname, mcontent, kvlist) in metadata:
            attr = self.build_dict(kvlist)
            self.metadata.append((mname, mcontent, attr))

    def get_manifest(self):
        manlist = []
        for (id, href, mtype, attr) in self.manifest:
            kvlist = self.xlate_dict(attr)
            manlist.append((id, href, mtype, kvlist))
        return manlist

    def set_manifest(self, manlist):
        self.manifest = []
        for (id, href, mtype, kvlist) in manlist:
            attr = self.build_dict(kvlist)
            self.manifest.append((id, href, mtype, attr))

    def get_spine_attr(self):
        kvlist = self.xlate_dict(self.spine_attr)
        return kvlist

    def set_spine_attr(self, kvlist):
        attr = self.build_dict(kvlist)
        self.spine_attr = attr

    def get_spine(self):
        spine = []
        for (idref, attr) in self.spine:
            kvlist = self.xlate_dict(attr)
            spine.append((idref, kvlist))
        return spine

    def set_spine(self, splist):
        self.spine = []
        for (idref, kvlist) in splist:
            attr = self.build_dict(kvlist)
            self.spine.append((idref, attr))

    def get_guide(self):
        guide = []
        for (gtype, gtitle,  ghref) in self.guide:
            guide.append((gtype, gtitle, ghref))
        return guide

    def set_guide(self, gulist):
        self.guide = []
        for (gtype, gtitle,  ghref) in self.gulist:
            self.guide.append((gtype, gtitle, ghref))

    def get_bindings(self):
        bindings = []
        for (mtype, handler) in self.bindings:
            bindings.append((mtype, handler))
        return bindings

    def set_bindings(self, bindlist):
        self.bindings = []
        for (mtype, handler) in bindlist:
            self.bindings.append((mtype, handler))

    def convert_package_to_xml(self):
        xmlres = []
        (ver, uid, attr) = self.package
        xmlres.append('<package version="%s" unique-identifier="%s"' % (ver, uid))
        for key in attr:
            val = attr[key]
            val = xmlencode(val)
            xmlres.append(' %s="%s"' % (key, val))
        xmlres.append('>\n')
        return "".join(xmlres)

    def convert_metadata_attr_to_xml(self):
        xmlres = []
        attr = self.metadata_attr
        xmlres.append('  <metadata')
        for key in attr:
            val= attr[key]
            val= xmlencode(val)
            xmlres.append(' %s="%s"' % (key, val))
        xmlres.append('>\n')
        return "".join(xmlres)

    def convert_metadata_entries_to_xml(self):
        xmlres = []
        for (mname, mcontent, attr) in self.metadata:
            xmlres.append('    <%s' % mname)
            for key in attr:
                val= attr[key]
                val= xmlencode(val)
                xmlres.append(' %s="%s"' % (key, val))
            if mcontent is None or mcontent == "":
                xmlres.append('/>\n')
            else:
                content= xmlencode(mcontent)
                xmlres.append('>%s</%s>\n' % (content, mname))
        return "".join(xmlres)

    def convert_manifest_entries_to_xml(self):
        xmlres = []
        for (id, href, mtype, attr) in self.manifest:
            # all hrefs should be kept in quoted (encoded) form
            xmlres.append('    <item id="%s" href="%s" media-type="%s"' % (id, href, mtype))
            for key in attr:
                val= attr[key]
                val= xmlencode(val)
                xmlres.append(' %s="%s"' % (key, val))
            xmlres.append('/>\n')
        return "".join(xmlres)

    def convert_spine_attr_to_xml(self):
        xmlres = []
        attr = self.spine_attr
        xmlres.append('  <spine')
        if attr is not None:
            for key in attr:
                val= attr[key]
                val= xmlencode(val)
                xmlres.append(' %s="%s"' % (key, val))
        xmlres.append('>\n')
        return "".join(xmlres)

    def convert_spine_entries_to_xml(self):
        xmlres=[]
        for (idref, attr) in self.spine:
            xmlres.append('    <itemref idref="%s"' % idref)
            if attr is not None:
                for key in attr:
                    val= attr[key]
                    val= xmlencode(val)
                    xmlres.append(' %s="%s"' % (key, val))
            xmlres.append('/>\n')
        return "".join(xmlres)

    def convert_guide_entries_to_xml(self):
        xmlres=[]
        for (gtype, gtitle, ghref) in self.guide:
            # all hrefs should already be in quoted (encoded) form
            xmlres.append('    <reference type="%s" title="%s" href="%s"/>\n' % (gtype, gtitle, ghref))
        return "".join(xmlres)

    def convert_binding_entries_to_xml(self):
        xmlres=[]
        for (mtype, handler) in self.bindings:
            xmlres.append('  <mediaType media-type="%s" handler="%s"/>\n' % (mtype, handler))
        return "".join(xmlres)

    def rebuild_opfxml(self):
        xmlres=[]
        xmlres.append('<?xml version="1.0" encoding="utf-8"?>\n')
        xmlres.append(self.convert_package_to_xml())
        xmlres.append(self.convert_metadata_attr_to_xml())
        xmlres.append(self.convert_metadata_entries_to_xml())
        xmlres.append('  </metadata>\n')
        xmlres.append('  <manifest>\n')
        xmlres.append(self.convert_manifest_entries_to_xml())
        xmlres.append('  </manifest>\n')
        xmlres.append(self.convert_spine_attr_to_xml())
        xmlres.append(self.convert_spine_entries_to_xml())
        xmlres.append('  </spine>\n')
        (opfver, uid, attr) = self.package
        if len(self.guide) > 0:
            xmlres.append('  <guide>\n')
            xmlres.append(self.convert_guide_entries_to_xml())
            xmlres.append('  </guide>\n')
        if len(self.bindings) > 0 and opfver.startswith('3'):
            xmlres.append('  <bindings>\n')
            xmlres.append(self.convert_binding_entries_to_xml())
            xmlres.append('  </bindings>\n')
        xmlres.append('</package>\n')
        return "".join(xmlres)


def parseopf(opfdata):
    opfparser = Opf_Parser(opfdata)
    return opfparser



def main():
    argv = sys.argv
    if len(argv) < 2:
        sys.exit(0)

    if not os.path.exists(argv[1]):
        sys.exit(0)

    with open(argv[1], 'rb') as f:
        data = f.read()
        data = data.decode('utf-8')

    op = parseopf(data)
    # print(op.get_package())
    # print(op.get_metadata_attr())
    # print(op.get_metadata())
    # print(op.get_manifest())
    # print(op.get_spine_attr())
    # print(op.get_spine())
    # print(op.get_guide())
    # print(op.get_bindings())
    print(op.rebuild_opfxml())
    return 0

if __name__ == '__main__':
    sys.exit(main())
