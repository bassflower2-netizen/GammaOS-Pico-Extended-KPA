"""Original procedural Atari Lynx-inspired hardware illustration and text badge.
No external image or logo assets; generated with Pillow during the build."""
from PIL import Image, ImageDraw, ImageFont
import sys

def logo():
    im=Image.new("RGBA",(640,180))
    d=ImageDraw.Draw(im)
    font=ImageFont.truetype("DejaVuSans-Bold.ttf",100)
    d.text((320,90),"ATARI LYNX",font=font,anchor="mm",fill="white",stroke_width=0)
    return im.crop(im.getbbox())

def hardware():
    im=Image.new("RGBA",(900,410))
    d=ImageDraw.Draw(im)
    d.rounded_rectangle((23,35,877,375),radius=87,fill="#1e2026",outline="#717780",width=5)
    d.rounded_rectangle((48,57,852,353),radius=70,fill="#41434a",outline="#14151a",width=3)
    d.rounded_rectangle((228,76,672,333),radius=25,fill="#17191d",outline="#666c70",width=4)
    d.rounded_rectangle((257,96,643,315),radius=12,fill="#090e12")
    d.rectangle((280,111,620,300),fill="#6e8d7a",outline="#090e12",width=3)
    d.polygon([(94,148),(135,148),(135,106),(181,106),(181,148),(222,148),(222,194),(181,194),(181,236),(135,236),(135,194),(94,194)],fill="#202127",outline="#90939a",width=3)
    for x,y,label in [(753,155,"A"),(804,218,"B")]:
        d.ellipse((x-29,y-29,x+29,y+29),fill="#802b34",outline="#bf6770",width=4)
        d.text((x,y),label,font=ImageFont.truetype("DejaVuSans-Bold.ttf",26),anchor="mm",fill="#fff4f2")
    for x in (112,177,731,796):
        d.rounded_rectangle((x-19,283,x+19,299),radius=7,fill="#15171a",outline="#797c82")
    for y in range(92,128,8):
        d.line((737,y,801,y),fill="#181a20",width=3)
    d.text((450,48),"ATARI LYNX",font=ImageFont.truetype("DejaVuSans-Bold.ttf",19),anchor="mm",fill="#d7dadd")
    return im.crop(im.getbbox())

if __name__=="__main__":
    logo().save(sys.argv[1])
