#! /usr/bin/env python

"""
mw2html - Mediawiki to static HTML

I use this to create a personal website from a local mediawiki
installation.  No search functionality.  Hacks the Monobook skin and
the produced HTML.

Connelly Barnes 2005.  Public domain.

Reworked by Andre Pinto 2009.
Improved performance.
Improved filtering.
Improved usability.
Customized for Audacity's manual wiki.
...
"""

__version__ = '0.1.0.0'

import re
import sys
import getopt
import random
import urllib
import textwrap
import urlparse
import os, os.path
import errno
import hashlib
import httplib
#import pdb
from time import strftime

try:
  set
except:
  from sets import Set as set

try:
  import htmldata
except:
  print 'Requires Python htmldata module:'
  print '  http://www.connellybarnes.com/code/htmldata/'
  sys.exit()

config             = None
MOVE_HREF          = 'movehref'
MADE_BY_COMMENT    = '<!-- Content generated by Mediawiki and mw2html -->'
INDEX_HTML         = 'index.html'
url_filename_cache = {}
redir_cache        = {}
wrote_file_set     = set()
sidebar_html       = ''
footer_text        = ''
counter            = 0
errors             = 0
conn               = None
domain             = ''

MONOBOOK_SKIN      = 'monobook'    # Constant identifier for Monobook.

class Config:
  """
  Instances contain all options passed at the command line.
  """
  def __init__(self, rooturl, outdir,
               flatten=True, index=None, clean=True,
               sidebar=None, hack_skin=True,
               made_by=True, overwrite=False, footer=None,
               skin=MONOBOOK_SKIN, move_href=True,
               remove_png=True, remove_history=True, limit_parent=False,
               special_mode=False, debug=False, no_images=False):
    self.rooturl         = rooturl
    self.outdir          = os.path.abspath(outdir)
    self.flatten         = flatten
    self.index           = index
    self.clean           = clean
    self.sidebar         = sidebar
    self.hack_skin       = hack_skin
    self.made_by         = made_by
    self.overwrite       = overwrite
    self.footer          = footer
    self.skin            = skin
    self.move_href       = move_href
    if self.sidebar is not None:
      self.sidebar       = os.path.abspath(self.sidebar)
    if self.footer is not None:
      self.footer        = os.path.abspath(self.footer)
    self.remove_png      = remove_png
    self.remove_history  = remove_history
    self.limit_parent    = limit_parent
    self.special_mode    = special_mode
    self.debug           = debug
    self.no_images       = no_images



def get_domain(u):
  """
  Get domain of URL.
  """
  url = normalize_url(u)
  
  #ParseResult(scheme='http', netloc='www.cwi.nl:80', path='/%7Eguido/Python.html', params='', query='', fragment='')
  L = list(urlparse.urlparse(url))
  
  return L[1]

def normalize_url(url, lower=True):
# url normalization - only for local comparison operations, use original url for online requests
  url = split_section(url)[0] 
  
  if lower:
    url = url.lower()
  
  if url.startswith('http://'):
    url = url[len('http://'):]
  if url.startswith('www.'):
    url = url[len('www.'):]
  
  url = url.strip('/')
  
  url = 'http://' + url
  
  urlparse.urljoin(config.rooturl, url)
  
  return url

def find_tag_limits(doc, filter_string, end_tag, start_tag, start_point = 0):
# find tag limits - start_string must be an unique identifier within doc

  i1 = doc.find(filter_string, start_point)

  if i1 == -1:
    return (-1,-1)

  aux   = doc.rfind(start_tag, start_point, i1+len(filter_string))
  
  # we've found the filter_string but it has not the start_tag, so we return a different value
  # telling the script to keep searching starting on the end of the filter_string found
  if aux == -1:
    return (-2, i1+len(filter_string))

  i1 = aux
  sdiv = i1
  ediv = i1 + len(start_tag)
  while(sdiv < ediv and sdiv != -1):
      sdiv = doc.find(start_tag, sdiv+len(start_tag))
      ediv = doc.find(end_tag  , ediv+len(end_tag))

  return (i1, ediv)

def clean_tag(doc, filter_string, end_tag, start_tag):
  #clean tagged text function
  start_point = 0
  while True:
    (start1, start2) = find_tag_limits(doc, filter_string, end_tag, start_tag, start_point)
    if start1 == -1 or start2 == -1:
      return doc
    if start1 == -2:
      start_point = start2
      continue
    end1 = doc.find('>', start1)+1;
    end2 = start2 + len(end_tag);
    doc = doc[:start1]+doc[end1:start2]+doc[end2:]
  
def remove_tag(doc, start_string, end_tag, start_tag):
  #remove tagged text function
  while True:
    (i1, i2) = find_tag_limits(doc, start_string, end_tag, start_tag)
    if i1 == -1 or i2 == -1:
      return doc
    doc = doc[:i1]+doc[i2+len(end_tag):]
 
def monobook_fix_html(doc, page_url):
  """
  Sets sidebar for Mediawiki 1.4beta6 Monobook HTML output.
  """
  global config
 
  if config.made_by:
    doc = doc.replace('<html xmlns=', MADE_BY_COMMENT + '\n<html xmlns=')

  doc = remove_tag(doc, '<div class="portlet" id="p-personal">', '</div>', '<div')
  doc = remove_tag(doc, '<div id="p-search" class="portlet">', '</div>','<div')
  doc = remove_tag(doc, '<div class="portlet" id="p-editors">', '</div>', '<div')
  
  #andre special mode
  if config.special_mode:
    # Remove ul list
    doc = remove_tag(doc,'<ul id="f-list">','</ul>', '<ul')

    # Remove link rel alternate and edit
    doc = re.sub(r'<link rel="alternate"[\s\S]+?/>',r'',doc)
    doc = re.sub(r'<link rel="edit"[\s\S]+?/>',r'',doc)
    
    # Remove print footer
    doc = re.sub(r'<div class="printfooter">[\s\S]+?</div>',r'',doc)
 
    # Remove noexport
    doc = remove_tag(doc,'<div class="noexport"','</div>', '<div')

    # Remove editornote
    doc = remove_tag(doc,'<div class="editornote"','</div>', '<div')

  else:
    # Remove powered by MediaWiki logo
    doc = re.sub(
      r'<div id="f-poweredbyico">[\s\S]+?(<ul id="f-list">)',
      r'\1', doc)

    # Remove page has been accessed X times list item.
    doc = re.sub(r'<li id="f-viewcount">[\s\S]+?</li>', r'', doc)

    # Remove disclaimers list item.
    doc = re.sub(r'<li id="f-disclaimer">[\s\S]+?</li>', r'', doc)
      
  # Remove edit links
  doc = remove_tag(doc, '<div class="editsection"', '</div>', '<div')
  doc = remove_tag(doc, '<span class="editsection"', '</span>', '<span')

  return doc
  
def pre_html_transform(doc, url):
  """
  User-customizable HTML transform.

  Given an HTML document (with URLs already rewritten), returns
  modified HTML document.
  """
  global config
  new_urls = []
  
  if config.hack_skin:
    if config.skin == MONOBOOK_SKIN:
      doc = monobook_fix_html(doc, url)
      if not config.special_mode:
        doc = monobook_hack_skin_html(doc)
    else:
      raise ValueError('unknown skin')
  
  if config.move_href:
    doc = fix_move_href_tags(doc)
  if config.remove_history:
    doc = html_remove_image_history(doc)
  
  return doc
  
def pos_html_transform(doc, url):
  global footer_text, config, sidebar_html
  url = normalize_url(url, False)
  
  # Add sidebar.html
  if config.sidebar != None and sidebar_html == '':
    f = open(config.sidebar, 'rU')
    sidebar_html = f.read()
    f.close()

  doc = re.sub( r'(<!-- end of the left \(by default at least\) column -->)', sidebar_html + r'\1', doc)
  
  # Remove empty links
  doc = clean_tag(doc, 'href=""', '</a>', '<a ');
  
  if config.special_mode:
    # Remove external link rel stylesheet
    doc = re.sub(r'<link rel="stylesheet" href="http://[\s\S]+?/>',r'',doc)
    
    # Remove external javascript
    doc = re.sub(r'<script type="text/javascript" src="http://[\s\S]+?</script>',r'',doc)
    
  # Replace remaining text with footer, if available (this needs to be done after parse_html to avoid rewriting of urls
  if config.footer is not None:
    s1 = '<div id="footer">'

  # match correct divs
  (i1,i2) = find_tag_limits(doc, s1, '</div>', '<div')

  if (i1 == -1):
    return doc

  if footer_text == '':
    f = open(config.footer, 'rU')
    footer_text = f.read()
    f.close()
  
  # add static dump time
  footer_html = footer_text.replace('%DATE%', strftime("%Y-%m-%d %H:%M:%S"))
  
  # add online url
  footer_html = footer_html.replace('%ONLINEURL%', url)

  if config.special_mode:
    # keep MediaWiki credits
    doc = doc[:i2] + footer_html + doc[i2:]
  else:
    doc = doc[:i1+len(s1)] + footer_html + doc[i2:]
    
  return doc

def fix_move_href_tags(doc):
  """
  Return copy of doc with all MOVE_HREF tags removed.
  """
  while '<' + MOVE_HREF in doc:
    i1 = doc.index('<' + MOVE_HREF)
    i2 = doc.index('</' + MOVE_HREF, i1+1)
    i3 = doc.index('>', i2+1)
    (start, end) = (i1, i3+1)
    tags = htmldata.tagextract(doc[start:end])
    assert tags[0][0] == MOVE_HREF
    assert tags[-1][0] == '/' + MOVE_HREF
    href = tags[0][1].get('href', '')
    new_tags = []
    for tag in tags[1:-1]:
      if len(tag) == 2:
        if 'href' in tag[1]:
          if href == '':
            continue
          tag[1]['href'] = href
      new_tags += [tag]
    doc = doc[:start] + htmldata.tagjoin(new_tags) + doc[end:]
  return doc

def html_remove_image_history(doc):
  """
  Remove image history and links to information.
  """
  doc = re.sub(r'<h2>Image history</h2>[\s\S]+?</ul>', r'', doc)
  doc = re.sub(r'<h2>Image links</h2>[\s\S]+?</ul>', r'', doc)
  return doc

def monobook_hack_skin_html(doc):
  """
  Hacks Monobook HTML output: use CSS ids for hacked skin.

  See monobook_hack_skin_css.
  """
  doc = doc.replace('<div id="globalWrapper">', '<div id="globalWrapperHacked">')
  doc = doc.replace('<div id="footer">', '<div id="footerHacked">')
  doc = doc.replace('</body>', '<br></body>')  
  return doc

def monobook_hack_skin_css(doc, url):
  """
  Hacks Mediawiki 1.4beta6 Monobook main CSS file for better looks.

  Removes flower background.  Defines *Hacked CSS ids, so we can add
  an orange bar at the top, and clear the orange bar right above the
  footer.
  """
  global config
  
  if not url.endswith('monobook/main.css'):
    return doc

  doc = "/* Monobook skin automatically modified by mw2html. */" + doc
  doc = doc.replace('url("headbg.jpg")', '')

  doc += """
    /* Begin hacks by mw2html */

    #globalWrapperHacked {
      font-size:127%;
      width: 100%;
      background-color: White;
      border-top: 1px solid #fabd23;
      border-bottom: 1px solid #fabd23;
      margin: 0.6em 0em 1em 0em;
      padding: 0em 0em 1.2em 0em;
    }

    #footerHacked {
      background-color: White;
      margin: 0.6em 0em 0em 0em;
      padding: 0.4em 0em 0em 0em;
      text-align: center;
      font-size: 90%;
    }

    #footerHacked li {
      display: inline;
      margin: 0 1.3em;
    }
    """

  c1 = '#column-one { padding-top: 160px; }'
  c2 = '#column-one { padding-top: 3.0em; }'
  assert c1 in doc

  doc = doc.replace(c1, '/* edit by mw2html */\n' + c2 +
                        '\n/* end edit by mw2html */\n')

  # Remove external link icons.
  if config.remove_png:
    doc = re.sub(r'#bodyContent a\[href \^="http://"\][\s\S]+?\}', r'', doc)

  return doc

def post_css_transform(doc, url):
  """
  User-customizable CSS transform.

  Given a CSS document (with URLs already rewritten), returns
  modified CSS document.
  """
  global config
  
  if config.hack_skin and not config.special_mode:
    if config.skin == MONOBOOK_SKIN:
      doc = monobook_hack_skin_css(doc, url)
    else:
      raise ValueError('unknown skin')
  return doc
  
def move_to_index_if_needed(ans):
  global config
  if ans.endswith(config.index):
    ans = ans[:len(ans)-len(config.index)] + INDEX_HTML
  return ans

def file_exists_in_written_set(filename):
  return os.path.normcase(os.path.normpath(filename)) in wrote_file_set

def find_unused_filename(filename, exists=os.path.exists):
  """
  Return 'file' if 'file' doesn't exist, otherwise 'file1', 'file2', etc.

  Existance is determined by the callable exists(), which takes
  a filename and returns a boolean.
  """
  if not exists(filename):
    return filename
  (head, tail) = os.path.split(filename)
  i = 1
  while True:
    numbered = (os.path.splitext(tail)[0] + str(i) +
                os.path.splitext(tail)[1])
    fullname = os.path.join(head, numbered)
    if not exists(fullname):
      return fullname
    i += 1

def clean_filename(url, ans):
  # Split outdir and our file/dir under outdir
  # (Note: ans may not be a valid filename)
  global config
  
  (par, ans) = (ans[:len(config.outdir)], ans[len(config.outdir):])
  if ans.startswith(os.sep):
    ans = ans[1:]

  # Replace % escape codes with underscores, dashes with underscores.
  while '%%' in ans:
    ans = ans[:ans.index('%%')] + '_' + ans[ans.index('%%')+2:]
  while '%25' in ans:
    ans = ans[:ans.index('%25')] + '_' + ans[ans.index('%25')+5:]
  while '%' in ans:
    ans = ans[:ans.index('%')] + '_' + ans[ans.index('%')+3:]
  ans = ans.replace('-', '_')
  while '__' in ans:
    ans = ans.replace('__', '_')
  while '_.' in ans:
    ans = ans.replace('_.', '.')

  # Rename math thumbnails
  if '/math/' in url:
    tail = os.path.split(ans)[1]
    if os.path.splitext(tail)[1] == '.png':
      tail = os.path.splitext(tail)[0]
      if set(tail) <= set('0123456789abcdef') and len(tail) == 32:
        ans = 'math_' + hashlib.md5(tail).hexdigest()[:4] + '.png'
  return os.path.join(par, ans)

def flatten_filename(url, filename):
  global config
  def get_fullname(relname):
    return os.path.join(config.outdir, relname)

  orig_ext = os.path.splitext(filename)[1]
  (head, tail) = os.path.split(filename)
  if tail == INDEX_HTML:
    (head, tail) = os.path.split(head)
  ans = tail
  if os.path.splitext(ans)[1] != orig_ext:
    ans = os.path.splitext(ans)[0] + orig_ext
  return os.path.join(config.outdir, ans)

def split_section(url):
  """
  Splits into (head, tail), where head contains no '#' and is max length.
  """
  if '#' in url:
    i = url.index('#')
    return (url[:i], url[i:])
  else:
    return (url, '')
	
def url_open(url):
  # download a file and retrieve its content and mimetype
  global conn, domain, counter, redir_cache, errors
 
  l_redir = []
  redirect = url
  while redirect != '':
    l_redir += [url]

    L = urlparse.urlparse(url)
    if L[1] != domain:
      conn.close()
      print "connection to",domain,"closed."
      conn = httplib.HTTPConnection(L[1])
      domain = L[1]
      print "connection to",domain,"opened."

    rel_url = url   
    pos = url.find(domain)
    if pos != -1:
      rel_url = url[pos+len(domain):]
    
    attempts = 0
    #number of attempts
    total_attempts = 3
    recovered = False
    success = False
    
    while not success and attempts < total_attempts:
      #increment httplib requests counter
      counter += 1
      try:
        conn.request("GET", rel_url)
        r = conn.getresponse()
        print 'Status',r.status,r.reason,'accessing',rel_url
        if r.status == 404:
          print "it's not possible to recover this error."
          errors += 1
          return ('','')
        if r.status == 500:
          print "eventually this error might be recovered. let's try again."
          print 'reconnecting...'
          conn = httplib.HTTPConnection(domain)
          attempts += 1
          continue
        if attempts != 0:
          recovered = True
        success = True
        
      except httplib.HTTPException, e:
        print 'ERROR',e.__class__.__name__,'while retrieving', url
        conn.close
        if e.__class__.__name__ in ['BadStatusLine', 'ImproperConnectionState', 'NotConnected', 'IncompleteRead', 'ResponseNotReady']:
          print "eventually this error might be recovered. let's try again."
          print 'reconnecting...'
          conn = httplib.HTTPConnection(domain)
          attempts += 1
        else:
          print "it's not possible to recover this error."
          errors += 1
          return ('','')

    if recovered:
      print "error recovered"

    if not success:
      print "it was not possible to recover this error."
      errors += 1
      return ('', '')
      
    redirect = r.getheader('Location', '').split(';')[0]
  
    if redirect != "":
      url = redirect
    else:
      doc = r.read()
  
  for item in l_redir:
    redir_cache[normalize_url(item)] = normalize_url(url)
  
  mimetype = r.getheader('Content-Type', '').split(';')[0].lower()

  return (doc, mimetype)
  
def url_to_filename(url):
  """
  Translate a full url to a full filename (in local OS format) under outdir.
  Transforms web url into local url and caches it.
  Downloads the file to disk and works with it there instead of download the same file two times (Performance Improvement).
  """ 
  global config
  nurl = normalize_url(url)
  
  if nurl in url_filename_cache:
    return url_filename_cache[nurl]

  #ParseResult(scheme='http', netloc='www.cwi.nl:80', path='/%7Eguido/Python.html', params='', query='', fragment='')
  L = list(urlparse.urlparse(nurl))
  
  #this way the url will not create a folder outside of the maindomain
  droot = get_domain(config.rooturl)
  if (L[1] != droot):
    L[1] = droot

  L[2] = L[2].strip('/')
  lpath = L[2].split('/')
  if not '.' in lpath[-1]:
    # url ends with a directory name.  Store it under index.html.
    L[2] += '/' + INDEX_HTML
  else:
    # 'title=' parsing
    if L[4].startswith('title=') and L[2].endswith('index.php'):
      L[4] = L[4][len('title='):]
      L[2] = L[2][:-len('index.php')]

  L[2] = L[2].strip('/')
  
  #don't sanitize / for path
  L[0] = ''
  L[2] = urllib.quote_plus(L[2], '/')
  L[3] = urllib.quote_plus(L[3])
  L[4] = urllib.quote_plus(L[4])
  L[5] = urllib.quote_plus(L[5])

  # Local filename relative to outdir
  # os.sep - O.S. directory separator
  # (More transformations are made to this below...).
  FL = []
  for i in L:
    if i != '':
      FL += [i]
  
  subfile = os.sep.join(FL)

  (doc, mimetype) = url_open(url)
  if doc == '' or mimetype == '':
    url_filename_cache[nurl] = ''
    return ''

  # Fix up extension based on mime type.
  # Maps mimetype to file extension
  MIME_MAP = {
   'image/jpeg': 'jpg', 'image/png': 'png', 'image/gif': 'gif',
   'image/tiff': 'tiff', 'text/plain': 'txt', 'text/html': 'html',
   'text/rtf': 'rtf', 'text/css': 'css', 'text/sgml': 'sgml',
   'text/xml': 'xml', 'application/zip': 'zip'
  }

  if mimetype in MIME_MAP:
    (root, ext) = os.path.splitext(subfile)
    ext = '.' + MIME_MAP[mimetype]
    subfile = root + ext

  subfile = subfile.lower()
  
  ans = os.path.join(config.outdir, subfile)

  if config.flatten:
    ans = flatten_filename(nurl, ans)

  if config.clean:
    ans = clean_filename(nurl, ans)

  if config.index != None:
    ans = move_to_index_if_needed(ans)
	
  ans = find_unused_filename(ans, file_exists_in_written_set)

  # Cache and return answer.
  wrote_file_set.add(os.path.normcase(os.path.normpath(ans)))
  url_filename_cache[nurl] = ans
  
  mode = ['wb', 'w'][mimetype.startswith('text')]

  # Make parent directory if it doesn't exist.
  try:
    os.makedirs(os.path.split(ans)[0])
  except OSError, e:
    if e.errno != errno.EEXIST:
      raise

  # Not really needed since we checked that the directory
  # outdir didn't exist at the top of run(), but let's double check.
  if os.path.exists(ans) and not config.overwrite:
    out.write('File already exists: ' + str(ans))
    sys.exit(1)

  f = open(ans, mode)
  f.write(doc)
  f.close()

  return ans

def url_to_relative(url, cururl):
  """
  Translate a full url to a filename (in URL format) relative to cururl.
  Relative url from curul to url.
  """

  cururl = split_section(cururl)[0]
  (url, section) = split_section(url)
 
  L1 = url_to_filename(url).replace(os.sep, '/').strip('/').split('/')
  if L1 == '':
    return ''

  L2 = url_to_filename(cururl).replace(os.sep, '/').strip('/').split('/')

  while L1 != [] and L2 != [] and L1[0] == L2[0]:
    L1 = L1[1:]
    L2 = L2[1:]

  rel_url = urllib.quote('../' * (len(L2) - 1) + '/'.join(L1)) + section
  if rel_url == '':
    return '#'
  else:
    return rel_url

def parse_css(doc, url):
  """
  Returns (modified_doc, new_urls), where new_urls are absolute URLs for
  all links found in the CSS.
  """
  global config
  
  new_urls = []  

  L = htmldata.urlextract(doc, url, 'text/css')
  for item in L:
    # Store url locally.
    u = item.url
    
    if config.no_images and any(u.strip().lower().endswith(suffix) for suffix in ('.jpg', '.gif', '.png', '.ico')):
      item.url = ''
      continue

    new_urls += [u]
    item.url = url_to_relative(u, url)

  newdoc = htmldata.urljoin(doc, L)
  newdoc = post_css_transform(newdoc, url)

  return (newdoc, new_urls)

def should_follow(url):
  """
  Returns a boolean for whether url should be spidered

  Given that 'url' was linked to from site, return whether
  'url' should be spidered as well.
  """
  global config

  # we don't have search on the local version
  if (url.endswith('#searchInput')):
    return False

  # False if different domains.
  nurl  = normalize_url(url)
  droot = get_domain(config.rooturl)
  dn    = get_domain(nurl) 
  if droot != dn and not (dn.endswith(droot) or droot.endswith(dn)):
    if config.debug:
      print url, 'not in the same domain'
    return False
  
  # False if multiple query fields or parameters found
  if (url.count('&') >= 1 or url.count(';') > 0) and not any(x in url for x in ('.css', 'gen=css')):
    if config.debug:
      print url, 'with multiple query fields'
    return False

  if any(x in url for x in ('Special:', 'Image:', 'Talk:', 'User:', 'Help:')):
    if config.debug:
      print url, 'is a forbidden wiki page'
    return False

  if config.no_images and any(url.strip().lower().endswith(suffix) for suffix in ('.jpg', '.gif', '.png', '.ico')):
    if config.debug:
      print url, 'is a image and you are in no-images mode'
    return False

  # limit_parent support
  ncurl = normalize_url(config.rooturl)
  
  if config.limit_parent and not nurl.startswith(ncurl):
    L = nurl.split('/')
    if ('.' not in L[-1]):
      if config.debug:
        print url, 'is a file outside of scope with unknown extension'
      return False

    forbidden_parents = ['.php','.html','.htm']
    for fp in forbidden_parents:
      if fp in L[-1]:
        if config.debug:
          print url, 'is a page outside of scope'
        return False
	
  return True

def parse_html(doc, url):
  """
  Returns (modified_doc, new_urls), where new_urls are absolute URLs for
  all links we want to spider in the HTML.
  """
  global config
  
  BEGIN_COMMENT_REPLACE = '<BEGINCOMMENT-' + str(random.random()) + '>'
  END_COMMENT_REPLACE   = '<ENDCOMMENT-' + str(random.random()) + '>'

  new_urls = []  

  doc = pre_html_transform(doc, url)  
  # Temporarily "get rid" of comments so htmldata will find the URLs
  # in the funky "<!--[if" HTML hackery for IE.
  doc = doc.replace('<!--', BEGIN_COMMENT_REPLACE)
  doc = doc.replace('-->', END_COMMENT_REPLACE)

  L = htmldata.urlextract(doc, url, 'text/html')

  for item in L:
    u = item.url
    follow = should_follow(u)
    if follow:
      if config.debug:
        print 'ACCEPTED   - ', u
      # Store url locally.
      new_urls += [u]
      item.url = url_to_relative(u, url)
    else:
      if not any( license in u for license in ('creativecommons.org', 'wxwidgets.org', 'gnu.org', 'mediawiki.org') ):
        item.url = ''
      if config.debug:
        print 'DENIED     - ', u

  newdoc = htmldata.urljoin(doc, L)
  newdoc = newdoc.replace(BEGIN_COMMENT_REPLACE, '<!--')
  newdoc = newdoc.replace(END_COMMENT_REPLACE, '-->')

  newdoc = pos_html_transform(newdoc, url)
  
  return (newdoc, new_urls)
  

def run(out=sys.stdout):
  """
  Code interface.
  """
  global conn, domain, counter, redir_cache, config
  
  if urlparse.urlparse(config.rooturl)[1].lower().endswith('wikipedia.org'):
    out.write('Please do not use robots with the Wikipedia site.\n')
    out.write('Instead, install the Wikipedia database locally and use mw2html on\n')
    out.write('your local installation.  See the Mediawiki site for more information.\n')
    sys.exit(1)

  # Number of files saved
  n = 0

  if not config.overwrite and os.path.exists(config.outdir):
    out.write('Error: Directory exists: ' + str(config.outdir) )
    sys.exit(1)

  domain = get_domain(config.rooturl)
  conn = httplib.HTTPConnection(domain)
  print 'connection established to:', domain
  complete = set()
  pending  = set([config.rooturl])
  
  start = True
  while len(pending) > 0:
    url = pending.pop()
    nurl = normalize_url(url)
    
    if nurl in redir_cache:
      nurl = redir_cache[nurl]

    if nurl in complete:
      if config.debug:
        print url, 'already processed'
      continue
 
    complete.add(nurl)
    filename = url_to_filename(url)

    #this is needed for the first path as it doesn't know if it is a redirect or not in the begining
    #at this point all the content of redir_cache is relative to the start path
    if start:
      start = False
      aux_url = ''
      for redir in redir_cache.iterkeys():
        aux_url = normalize_url(redir)
        url_filename_cache[aux_url] = filename
        if aux_url not in complete:
          complete.add(aux_url)
      if aux_url != '':
        nurl = normalize_url(redir_cache[nurl])
 
    if filename == '':
      continue

    if not os.path.exists(filename):
      print "ERROR: ", url, '\n'
      continue

    f        = open(filename, 'r')
    doc      = f.read()
    f.close()
    new_urls = []

    if filename.endswith('.html'):
      (doc, new_urls) = parse_html(doc, url)
    elif filename.endswith('.css'):
      (doc, new_urls) = parse_css(doc, url)

    # Enqueue URLs that we haven't yet spidered.
    for u in new_urls:
      if normalize_url(u) not in complete:
        # Strip off any #section link.
        if '#' in u:
          u = u[:u.index('#')]
        pending.add(u)

    # Save document changes to disk
    update = False
    text_ext = ( 'txt', 'html', 'rtf', 'css', 'sgml', 'xml' )
    for ext in text_ext:
      if filename.endswith(ext):
        update = True
        break
 
    if update:
      f = open(filename, 'w')
      f.write(doc)
      f.close()
 
    if config.debug:
      out.write(url + '\n => ' + filename + '\n\n')
    n += 1
  
  conn.close()
  print "connection to",domain,"closed."
  out.write(str(n) + ' files saved\n')
  print counter, "httplib requests done"
  print errors, "errors not recovered"


def usage():
  """
  Print command line options.
  """
  usage_str = """
  mw2html url outdir [options]

  MW2HTML Audacity version
  Converts an entire Mediawiki site into static HTML.
  WARNING: This is a recursive robot that ignores robots.txt.  Use with care.

    url                  - URL of mediawiki page to convert to static HTML.
    outdir               - Output directory.

    -f, --force          - Overwrite existing files in outdir.
    -d, --debug          - Debug mode.
    -s, --special-mode   - -f --no-flatten --limit-parent -l sidebar.html
                           -b footer.html, keeps MediaWiki icon and more
                           design changes.
    --no-flatten         - Do not flatten directory structure.
    --no-clean           - Do not clean up filenames (clean replaces
                           non-alphanumeric chars with _, renames math thumbs).
    --no-hack-skin       - Do not modify skin CSS and HTML for looks.
    --no-made-by         - Suppress "generated by" comment in HTML source.
    --no-move-href       - Disable <movehref> tag. [1]
    --no-remove-png      - Retain external link PNG icons.
    --no-remove-history  - Retain image history and links to information.
    --no-images          - Discard images
    --limit-parent       - Do not explore .php pages outside the url path 
                           (outside css, images and other files aren't affected)
    -l, --left=a.html    - Paste HTML fragment file into left sidebar.
    -t, --top=a.html     - Paste HTML fragment file into top horiz bar.
    -b, --bottom=a.html  - Paste HTML fragment file into footer horiz bar.
    -i, --index=filename - Move given filename in outdir to index.html.

  Example Usage:
    mw2html http://127.0.0.1/mywiki/ out -f -i main_page.html -l sidebar.html

    Freezes wiki into 'out' directory, moves main_page.html => index.html,
    assumes sidebar.html is defined in the current directory.

  [1]. The <movehref> tag.
       Wiki syntax: <html><movehref href="a"></html>...<html></movehref></html>.
       When enabled, this tag will cause all href= attributes inside of it to be
       set to the given location.  This is useful for linking images.
       In MediaWiki, for the <html> tag to work, one needs to enable $wgRawHtml
       and $wgWhitelistEdit in LocalSettings.php.  A <movehref> tag with no href
       field will remove all links inside it.

  """

  print textwrap.dedent(usage_str.strip('\n'))
  sys.exit(1)


def main():
  """
  Command line interface.
  """
  global config
  try:
    (opts, args) = getopt.gnu_getopt(sys.argv[1:], 'fsdl:t:b:i:',
                   ['force', 'no-flatten', 'no-clean',
                    'no-hack-skin', 'no-made-by', 'left=',
                    'top=', 'bottom=', 'index=', 'no-move-href',
                    'no-remove-png', 'no-remove-history', 'limit-parent',
                    'special-mode','debug','no-images'])
  except getopt.GetoptError:
    usage()

  # Parse non-option arguments
  try:
    (rooturl, outdir) = args
  except ValueError:
    usage()
  config = Config(rooturl=rooturl, outdir=outdir)

  # Parse option arguments
  for (opt, arg) in opts:
    if opt in ['-f', '--force', '-s', '-special-mode']:
      config.overwrite      = True
    if opt in ['--no-flatten', '-s', '-special-mode']:
      config.flatten        = False
    if opt in ['--no-clean']:
      config.clean          = False
    if opt in ['--no-hack-skin']:
      config.hack_skin      = False
    if opt in ['--no-made-by']:
      config.made_by        = False
    if opt in ['--no-move-href']:
      config.move_href      = False
    if opt in ['--no-remove-png']:
      config.remove_png     = False
    if opt in ['--no-remove-history']:
      config.remove_history = False
    if opt in ['--no-images']:
      config.no_images      = True
    if opt in ['--limit-parent', '-s', '-special-mode']:
      config.limit_parent   = True
    if opt in ['-s', '-special-mode']:
      config.special_mode   = True
      config.sidebar        = 'sidebar.html'
      config.footer         = 'footer.html'
    if opt in ['-d','--debug']:
      config.debug          = True
    if opt in ['-l', '--left']:
      config.sidebar        = os.path.abspath(arg)
    if opt in ['-t', '--top']:
      raise NotImplementedError
      config.header         = os.path.abspath(arg)
    if opt in ['-b', '--bottom']:
      config.footer         = os.path.abspath(arg)
    if opt in ['-i', '--index']:
      config.index          = arg

  # Run program
  run()


if __name__ == '__main__':
  main()