"""Generate the original, editable Glassgarden faceplate (no external assets)."""
from pathlib import Path
import math
out=[]
def add(s): out.append(s)
def text(x,y,s,size=24,spacing=3,color='#f5e4c2'):
    add(f'<text x="{x}" y="{y}" text-anchor="middle" fill="{color}" font-family="DejaVu Serif,Georgia,serif" font-size="{size}" letter-spacing="{spacing}">{s}</text>')
def circle(x,y,r,fill,stroke='none',sw=1):
    add(f'<circle cx="{x}" cy="{y}" r="{r}" fill="{fill}" stroke="{stroke}" stroke-width="{sw}"/>')
def line(x,y,a,b,color='#cfbf95',sw=2,opacity=1):
    add(f'<path d="M{x:.2f},{y:.2f} L{a:.2f},{b:.2f}" fill="none" stroke="{color}" stroke-width="{sw}" opacity="{opacity}" stroke-linecap="round"/>')
add('''<svg xmlns="http://www.w3.org/2000/svg" width="1024" height="1536" viewBox="0 0 1024 1536">
<defs>
<linearGradient id="case" x1="0" y1="0" x2="1" y2="1"><stop stop-color="#70927b"/><stop offset=".3" stop-color="#456d56"/><stop offset=".65" stop-color="#385a46"/><stop offset="1" stop-color="#263e32"/></linearGradient>
<linearGradient id="edge" x2="0" y2="1"><stop stop-color="#5e8066"/><stop offset=".5" stop-color="#263d30"/><stop offset="1" stop-color="#14281f"/></linearGradient>
<radialGradient id="knob"><stop stop-color="#3f4140"/><stop offset=".7" stop-color="#252928"/><stop offset=".93" stop-color="#131716"/><stop offset="1" stop-color="#080e0b"/></radialGradient>
<linearGradient id="rim" x2=".7" y2="1"><stop stop-color="#9d9f8c"/><stop offset=".23" stop-color="#373f36"/><stop offset=".5" stop-color="#1a241c"/><stop offset=".8" stop-color="#626858"/><stop offset="1" stop-color="#111b14"/></linearGradient>
<linearGradient id="chrome" x1="0" y1="0" x2="1" y2="1"><stop stop-color="#ffffef"/><stop offset=".18" stop-color="#d6d8ca"/><stop offset=".32" stop-color="#7a837b"/><stop offset=".43" stop-color="#f0f2de"/><stop offset=".57" stop-color="#a7afa2"/><stop offset=".75" stop-color="#626e65"/><stop offset=".88" stop-color="#e3e4cf"/><stop offset="1" stop-color="#7a877b"/></linearGradient>
<linearGradient id="button" x1=".15" y1="0" x2=".8" y2="1"><stop stop-color="#f3f5de"/><stop offset=".4" stop-color="#c9d0bf"/><stop offset="1" stop-color="#708477"/></linearGradient>
<filter id="grain"><feTurbulence type="fractalNoise" baseFrequency=".85" numOctaves="3" seed="27"/><feColorMatrix type="saturate" values="0"/><feComponentTransfer><feFuncA type="linear" slope=".065"/></feComponentTransfer><feBlend in="SourceGraphic" mode="soft-light"/></filter>
<filter id="shadow" x="-.3" y="-.2" width="1.6" height="1.5"><feGaussianBlur stdDeviation="13"/></filter>
</defs>
<rect width="1024" height="1536" fill="#111b17"/>
<rect x="88" y="148" width="870" height="1304" rx="53" fill="#030a06" opacity=".75" filter="url(#shadow)"/>
<rect x="74" y="125" width="876" height="1316" rx="48" fill="url(#edge)" stroke="#0c2116" stroke-width="3"/>
<rect x="80" y="117" width="864" height="1305" rx="43" fill="url(#case)" stroke="#90a488" stroke-width="2" filter="url(#grain)"/>
<rect x="101" y="140" width="822" height="1258" rx="29" fill="none" stroke="#d9c797" stroke-opacity=".58" stroke-width="2"/>
<rect x="107" y="146" width="810" height="1246" rx="24" fill="none" stroke="#1d3e2b" stroke-width="1"/>
''')
for x,y in [(128,167),(896,167),(128,1374),(896,1374)]:
    circle(x,y+3,16,'#1e3426');circle(x,y,15,'url(#chrome)','#2c4032',2)
    line(x-7,y+2,x+7,y-2,'#344b3c',3)
text(512,182,'G R A N U L A R',19,2)
text(512,444,'C L O U D',19,2)
for x,y,label in [(268,281,'PITCH'),(512,281,'SIZE'),(755,281,'SCATTER'),(268,543,'DENSITY'),(512,543,'FEEDBACK'),(755,543,'MIX')]:
    for i in range(25):
        a=math.radians(135+270*i/24);r=82;rr=92 if i%4==0 else 88
        line(x+math.cos(a)*r,y+math.sin(a)*r,x+math.cos(a)*rr,y+math.sin(a)*rr,'#dfd0a9',1.5,.88)
    circle(x,y+6,70,'#1b3224');circle(x,y,69,'url(#rim)','#0e2116',1)
    circle(x,y-7,61,'url(#knob)','#555a4a',1)
    for i in range(64):
        a=math.tau*i/64
        line(x+math.cos(a)*54,y-7+math.sin(a)*54,x+math.cos(a)*59,y-7+math.sin(a)*59,'#8a8c77' if i<32 else '#080e0b',1,.25)
    circle(x,y-7,50,'url(#knob)','#080e0b',1)
    add(f'<path d="M{x-2},{y-55} L{x+2},{y-55} L{x+2},{y-26} L{x-2},{y-26} Z" fill="#ecdfb9"/>')
    text(x,y+(94 if y==281 else 93),label,25,3)
for x,heading,left,right,bottom in [(325,'TEXTURE','MIST','SHARDS','GLASS'),(702,'HARMONY','SOLO','OCTAVE','FIFTH')]:
    text(x,701,heading,23,3)
    text(x-92,750,left,17,1);text(x+100,750,right,17,1);text(x,802,bottom,17,2)
    line(x-100,715,x+100,715,'#bcba91',1,.5)
# Sunrise and faceted glass garden: the family sun mark gains crystalline petals.
for i in range(19):
    a=math.pi+math.pi*i/18
    line(512+math.cos(a)*92,965+math.sin(a)*92,512+math.cos(a)*115,965+math.sin(a)*115,'#e5d3a5',1.6,.85)
add('<path d="M434,965 A78,78 0 0 1 590,965" fill="#daca9e" fill-opacity=".07" stroke="#e5d3a5" stroke-width="2"/>')
for y,w in [(971,354),(979,333),(988,296),(997,258)]:line(512-w,y,512+w,y,'#cdbd94',1.3,.65)
for x,y,w,h in [(476,931,26,74),(513,904,30,100),(550,944,24,59),(438,955,20,48),(585,963,16,42)]:
    add(f'<path d="M{x},{y} L{x+w},{y+h*.35} L{x+w*.8},{y+h} L{x-w*.75},{y+h} L{x-w},{y+h*.35} Z M{x},{y} L{x+w*.15},{y+h*.42} L{x+w*.8},{y+h} M{x+w*.15},{y+h*.42} L{x-w},{y+h*.35} M{x+w*.15},{y+h*.42} L{x+w},{y+h*.35} M{x+w*.15},{y+h*.42} L{x-w*.75},{y+h}" fill="#b4d6b0" fill-opacity=".08" stroke="#e5d3a5" stroke-width="1.4"/>')
text(512,1048,'SUNVEIL AUDIO',30,7)
text(512,1101,'GLASSGARDEN',44,5)
text(512,1131,'GRANULAR PITCH / STEREO CLOUD',15,2)
for x,label in [(260,'ENGAGE'),(512,'FREEZE'),(766,'REVERSE')]:
    text(x,1169,label,19,2)
    circle(x,1197,22,'#233e2c','#bdba9b',2)
    circle(x,1314,79,'#1b3224')
    circle(x,1304,78,'url(#chrome)','#253d2d',2)
    # Hexagonal nut, reflective concentric washer and sprung button.
    points=' '.join(f'{x+64*math.cos(math.tau*i/6):.2f},{1304+64*math.sin(math.tau*i/6):.2f}' for i in range(6))
    add(f'<polygon points="{points}" fill="url(#chrome)" stroke="#637565" stroke-width="2"/>')
    circle(x,1304,50,'#263d30','#e6e8d3',3)
    circle(x,1298,43,'url(#chrome)','#4c6554',2)
    circle(x,1291,35,'url(#button)','#dae0cd',2)
    circle(x,1291,28,'none','#a6b49f',1)
text(512,1511,'SUNVEIL AUDIO  /  1.0',12,3,'#7b9983')
add('</svg>')
path=Path(__file__).resolve().parents[1]/'Resources/Faceplate.svg'
path.write_text('\n'.join(out));print(path)
