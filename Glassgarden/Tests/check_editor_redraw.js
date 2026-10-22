// Optional visual regression check. Requires Node.js and the sharp package.
// Generate inputs with: GlassgardenPreview Resources/Faceplate.png <folder> --regression
const fs = require('fs');
const path = require('path');
const sharp = require(process.env.CODEX_PRIMARY_RUNTIME_NODE_MODULES
    ? path.join(process.env.CODEX_PRIMARY_RUNTIME_NODE_MODULES, 'sharp') : 'sharp');
(async () => {
    const folder=process.argv[2], plate=process.argv[3];
    if (!folder || !plate) throw new Error('Usage: node check_editor_redraw.js <folder> <Faceplate.png>');
    const plateBytes=fs.readFileSync(plate);
    const uri='data:image/png;base64,'+plateBytes.toString('base64');
    let cases=JSON.parse(fs.readFileSync(path.join(folder,'cases.json'),'utf8'));
    if (process.argv.includes('--controls')) {
        // Every visible parameter, focus-ring shape, momentary button, panel
        // transition and a preset change; larger-size samples cover each shape.
        const normal=new Set([
            ...Array.from({length:11},(_,i)=>'main-'+i),
            ...Array.from({length:8},(_,i)=>'details-'+(i+11)),
            'bypass-freeze','focus-0','focus-6','focus-7','focus-8','focus-10',
            'details-focus-11','details-focus-16','tap','details-clear',
            'open-details','close-details','load-preset',
            'main-0-return','main-10-return','details-16-return','details-18-return',
            'main-sync','details-sync'
        ]);
        const large=new Set(['main-0','main-7','main-10','details-16','details-sync',
            'details-focus-11','open-details','close-details']);
        cases=cases.filter(name=>name.endsWith('-600') ? normal.has(name.slice(0,-4))
            : name.endsWith('-1200') && large.has(name.slice(0,-5)));
    }
    // Match the native editor's small cached knob bitmaps. Without this, librsvg
    // repeatedly transforms the entire faceplate inside every clipped knob.
    const photoPattern=/<svg x='([^']+)' y='([^']+)' width='([^']+)' height='([^']+)' viewBox='([^']+)'><use xlink:href='#plate'\/><\/svg>/g;
    const sprites=new Map();
    const first=fs.readFileSync(path.join(folder,cases[0]+'-full.svg'),'utf8');
    for (const match of first.matchAll(photoPattern)) {
        if (sprites.has(match[5])) continue;
        const [left,top,width,height]=match[5].split(' ').map(Number);
        const bytes=await sharp(plateBytes).extract({left,top,width,height}).png().toBuffer();
        sprites.set(match[5],'data:image/png;base64,'+bytes.toString('base64'));
    }
    let completed=0;
    for (const name of cases) {
        const render=async suffix => {
            const svg=fs.readFileSync(path.join(folder,name+suffix+'.svg'),'utf8').replace('__FACEPLATE__',uri)
                .replace(photoPattern,(_,x,y,w,h,source)=>{
                    if(!sprites.has(source)) throw new Error('Uncached knob crop: '+source);
                    return `<image x='${x}' y='${y}' width='${w}' height='${h}' xlink:href='${sprites.get(source)}'/>`;
                });
            return sharp(Buffer.from(svg)).ensureAlpha().raw().toBuffer({resolveWithObject:true});
        };
        const expected=await render('-full'), actual=await render('-partial');
        // librsvg can round an antialiased edge by one 8-bit color level when
        // the same unchanged pixel is composited through a clipping group.
        let pixels=0;
        if (!expected.data.equals(actual.data)) {
            for(let i=0;i<expected.data.length;i+=4)
                if([0,1,2,3].some(c=>Math.abs(expected.data[i+c]-actual.data[i+c])>1)) ++pixels;
        }
        if (pixels) {
            await sharp(actual.data,{raw:actual.info}).png().toFile(path.join(folder,name+'-FAIL.png'));
            throw new Error(name+': '+pixels+' pixels differ after incremental repaint');
        }
        ++completed;
        if (completed%10===0) console.log('Checked '+completed+'/'+cases.length+' redraw states.');
    }
    console.log('PASS: '+cases.length+' incremental redraws match full rendering at 600x900 and 1200x1800 (within one 8-bit color level for antialiased edges).');
})().catch(error=>{console.error(error.message);process.exitCode=1;});
