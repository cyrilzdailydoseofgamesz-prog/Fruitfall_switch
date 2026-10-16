import struct,sys,os,wave
import numpy as np
F=[(0,0),(60,0),(115,-52),(98,-55),(122,-60)]
def dec(blocks):
    s1=s2=0;out=[]
    for b in blocks:
        sh=b[0]&15; pr=min((b[0]>>4)&15,4)
        f0,f1=F[pr]
        for byte in b[2:]:
            for nib in (byte&15,byte>>4):
                v=nib-16 if nib>=8 else nib
                s=(v<<12)>>sh if sh<=12 else (v<<12)
                s=s+((s1*f0+s2*f1+32)>>6)
                s=max(-32768,min(32767,s)); out.append(s); s2=s1;s1=s
    return np.array(out,dtype=np.int16)
def rough(x):
    x=x.astype(np.float64); return np.abs(np.diff(x)).mean()/(np.abs(x).mean()+1)
def load(path,il=None):
    d=open(path,'rb').read()
    ch=1; rate=struct.unpack('<I',d[12:16])[0]
    body=d[24:]; nb=len(body)//16
    blocks=[body[i*16:i*16+16] for i in range(nb)]
    if ch==1: return rate,[dec(blocks)],None
    best=None
    for il_ in ([il] if il else [1,2,4,8,16,32,64,128,256,512]):
        chs=[[],[]];i=0
        while i<nb:
            for c in range(2):
                chs[c]+=blocks[i:i+il_];i+=il_
        n=min(len(chs[0]),len(chs[1]))
        a,b=dec(chs[0][:n]),dec(chs[1][:n]);r=rough(a)+rough(b)
        if best is None or r<best[0]: best=(r,il_,[a,b])
    return rate,best[2],best[1]
if __name__=='__main__':
    src,dst=sys.argv[1],sys.argv[2];os.makedirs(dst,exist_ok=True)
    for fn in sorted(os.listdir(src)):
        if not fn.endswith('.SDF'):continue
        rate,chs,il=load(os.path.join(src,fn))
        n=min(len(c) for c in chs); data=np.stack([c[:n] for c in chs],axis=1).astype('<i2')
        w=wave.open(os.path.join(dst,fn[:-4].lower()+'.wav'),'wb');w.setnchannels(len(chs));w.setsampwidth(2);w.setframerate(rate);w.writeframes(data.tobytes());w.close()
        print(fn,len(chs),rate,'il',il,'secs %.1f'%(n/rate),'peak',int(np.abs(data).max()))
