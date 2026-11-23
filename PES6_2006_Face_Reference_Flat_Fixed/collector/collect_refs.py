#!/usr/bin/env python3
"""Collect 2005-2007 football reference photos from Wikimedia Commons.

Requires internet on YOUR machine. Uses Commons file-specific rights metadata.
Never downloads restricted/editorial or non-free photos.
"""
import argparse
import csv
import hashlib
import html
import json
import re
import sys
import time
import urllib.parse
import urllib.request
import urllib.error
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
API = 'https://commons.wikimedia.org/w/api.php'
USER_AGENT = 'PES6LegacyFaceReferenceResearch/1.0 (personal metadata-and-image pack; github.com/wikimedia/commons)'  # Set a contact UA if needed
MAX_BYTES = 25 * 1024 * 1024
LICENSES = ('cc by ', 'cc by-sa', 'cc0', 'public domain', 'pd-', 'public domain mark')
# Explicitly exclude licenses disallowing derivatives/commercial use.
DISALLOWED = ('by-nc', 'by-nd', 'all rights reserved', 'noncommercial', 'non-commercial', 'no derivatives')


def req(url, retries=3):
    last = None
    for attempt in range(retries):
        try:
            request = urllib.request.Request(url, headers={'User-Agent': USER_AGENT, 'Accept': 'application/json,image/jpeg,image/png,*/*;q=0.1'})
            return urllib.request.urlopen(request, timeout=35)
        except (urllib.error.HTTPError, urllib.error.URLError, TimeoutError) as e:
            last = e
            if isinstance(e, urllib.error.HTTPError) and e.code not in (429, 500, 502, 503, 504):
                break
            time.sleep(1.5 * (attempt + 1))
    raise RuntimeError(f'Network access failed: {last}')


def fetch_meta(title):
    args = {'action':'query','format':'json','formatversion':'2','titles':'File:'+title,
            'prop':'imageinfo','iiprop':'url|size|mime|extmetadata'}
    u = API + '?' + urllib.parse.urlencode(args)
    with req(u) as f:
        data = json.loads(f.read(2 * 1024 * 1024).decode('utf-8'))
    pages = data.get('query',{}).get('pages',[])
    if not pages or 'missing' in pages[0] or not pages[0].get('imageinfo'):
        raise RuntimeError('File missing from Commons: ' + title)
    return pages[0]['imageinfo'][0]


def ext(info, key):
    v = info.get('extmetadata',{}).get(key,{}).get('value','')
    return html.unescape(re.sub(r'<[^>]*>', '', v)).strip()


def permitted(info):
    mime = info.get('mime','')
    license_name = ext(info, 'LicenseShortName') or ext(info, 'License')
    text = license_name.lower()
    url = ext(info, 'LicenseUrl')
    if mime not in ('image/jpeg','image/png'):
        return False, f'unsupported mime {mime}', license_name, url
    if any(d in text or d in url.lower() for d in DISALLOWED):
        return False, 'license restrictions', license_name, url
    ok = ('cc by' in text or 'cc0' in text or 'public domain' in text or 'cc-by' in url.lower() or '/publicdomain/' in url.lower())
    if not ok: return False, 'unrecognized or missing license; inspect manually', license_name, url
    if info.get('size',0) > MAX_BYTES:
        return False, f'file too large ({info["size"]} bytes)', license_name, url
    return True, 'permitted by automated check; still review source page', license_name, url


def download(url, dest):
    parsed = urllib.parse.urlsplit(url)
    if parsed.scheme != 'https' or parsed.hostname != 'upload.wikimedia.org':
        raise RuntimeError('Unexpected file URL host (not upload.wikimedia.org)')
    dest.parent.mkdir(parents=True, exist_ok=True)
    tmp = dest.with_name(dest.name+'.part')
    with req(url) as f, tmp.open('wb') as out:
        n = 0
        while True:
            chunk = f.read(256*1024)
            if not chunk: break
            n += len(chunk)
            if n > MAX_BYTES: raise RuntimeError('File exceeded safety cap')
            out.write(chunk)
    sig = tmp.open('rb').read(16)
    if not (sig.startswith(b'\xff\xd8\xff') or sig.startswith(b'\x89PNG\r\n\x1a\n')):
        tmp.unlink(missing_ok=True)
        raise RuntimeError('Downloaded file is not a JPEG/PNG image')
    tmp.replace(dest)
    return n, hashlib.sha256(dest.read_bytes()).hexdigest()


def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('--no-download', action='store_true', help='Only generate rights-checked metadata (still uses internet)')
    ap.add_argument('--limit-per-player', type=int, default=2, help='Maximum reference images per player (default 2)')
    args=ap.parse_args()
    catalog=json.loads((ROOT/'collector'/'sources.json').read_text(encoding='utf-8'))
    outdir=ROOT/'downloaded_references'; outdir.mkdir(exist_ok=True)
    rows=[]; credits=[]; success=0
    fields=['team','player','reference_year','title','page_url','image_url','license','license_url','author','attribution','downloaded_file','sha256','width','height','view','quality_note','status']
    for team in catalog['teams']:
      for player in team['players']:
        count=0
        for cand in player['candidates']:
          title=cand['file']
          page='https://commons.wikimedia.org/wiki/File:'+urllib.parse.quote(title.replace(' ','_'))
          base=dict(team=team['name'],player=player['name'],reference_year=cand['year'],title=title,page_url=page,image_url='', license='',license_url='',author='',attribution='',downloaded_file='',sha256='',width='',height='',view=cand['view'],quality_note=cand['quality'],status='')
          if count>=args.limit_per_player: continue
          try:
            info=fetch_meta(title)
            ok, reason, lic, licurl=permitted(info)
            base.update(image_url=info.get('url',''), license=lic,license_url=licurl,author=ext(info,'Artist'),attribution=ext(info,'AttributionRequired'),width=info.get('width',''),height=info.get('height',''))
            if not ok:
                base['status']='SKIPPED: '+reason
                print('SKIP:',player['name'],title,reason)
            elif args.no_download:
                base['status']='LICENSE-CHECKED (not downloaded)';count+=1
            else:
                extn='.png' if info.get('mime')=='image/png' else '.jpg'
                dest=outdir/team['name']/player['id']/(re.sub(r'[^a-zA-Z0-9-]','_',title.rsplit('.',1)[0])[:80]+extn)
                length,digest=download(info['url'],dest)
                base.update(downloaded_file=str(dest.relative_to(ROOT)), sha256=digest,status='DOWNLOADED')
                success+=1;count+=1
                print('OK:',player['name'],title,f'{length} bytes')
            credits.append(f"- **{player['name']}** — [{title}]({page}), creator: {base['author'] or 'see source'}, license: [{lic or 'unverified'}]({licurl or page}). Downloaded: {base['status']}.")
          except Exception as e:
            base['status']='FAILED: '+str(e)[:230]
            print('FAIL:',player['name'],title,str(e)[:160])
          rows.append(base)
          time.sleep(0.3)  # Be polite to Commons
    with (ROOT/'COLLECTION_RESULT.csv').open('w',encoding='utf-8-sig',newline='') as f:
        w=csv.DictWriter(f,fieldnames=fields);w.writeheader();w.writerows(rows)
    (ROOT/'CREDITS_AND_LICENSES.md').write_text('# Photo credits and source/license review\n\nAll listed photos retain individual rights. Recheck licenses before distributing altered artworks. No implication of player or photographer endorsement.\n\n'+'\n'.join(credits)+'\n',encoding='utf-8')
    (ROOT/'DOWNLOAD_REPORT.json').write_text(json.dumps({'downloaded':success,'candidates_processed':len(rows),'statuses':{k:sum(1 for a in rows if a['status'].startswith(k)) for k in ('DOWNLOADED','SKIPPED','FAILED')}},indent=2), encoding='utf-8')
    result_zip=ROOT/'PES6_2006_Licensed_Photo_References.zip'
    with zipfile.ZipFile(result_zip,'w',compression=zipfile.ZIP_DEFLATED) as z:
      for p in sorted(outdir.rglob('*')):
        if p.is_file():z.write(p,p.relative_to(ROOT))
      for n in ('COLLECTION_RESULT.csv','CREDITS_AND_LICENSES.md','DOWNLOAD_REPORT.json','README.md','ANTIGRAVITY_PROMPT.md'):
        if (ROOT/n).is_file():z.write(ROOT/n,n)
    print('RESULT:',result_zip,'downloaded:',success,'source records:',len(rows))
    if success==0:
        print('No photos were fetched. Check network and the individual Commons file pages. The archive contains no photographs.')

if __name__=='__main__':
    main()
