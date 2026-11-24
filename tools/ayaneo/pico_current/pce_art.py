"""Original procedural PC Engine-inspired console art, generated during CI."""
from PIL import Image, ImageDraw, ImageFont
import sys
def logo():
    im=Image.new("RGBA",(640,180))
    d=ImageDraw.Draw(im)
    d.text((320,90),"PC ENGINE",font=ImageFont.truetype("DejaVuSans-Bold.ttf",96),anchor="mm",fill="white")
    return im.crop(im.getbbox())
def hardware():
    im=Image.new("RGBA",(820,440))
    d=ImageDraw.Draw(im)
    d.rounded_rectangle((35,32,785,408),radius=38,fill="#c8c6c4",outline="#77777d",width=5)
    d.rounded_rectangle((58,53,762,333),radius=29,fill="#ece9e5",outline="#b5b3b0",width=3)
    d.rounded_rectangle((208,86,612,240),radius=14,fill="#343339",outline="#77757c",width=3)
    d.rectangle((240,102,580,183),fill="#1d1c20")
    d.rounded_rectangle((253,195,567,221),radius=5,fill="#bfc1c4")
    d.rectangle((93,270,727,307),fill="#a53338")
    d.text((410,290),"PC ENGINE",font=ImageFont.truetype("DejaVuSans-Bold.ttf",25),anchor="mm",fill="white")
    d.rounded_rectangle((93,347,209,379),radius=9,fill="#38383d")
    d.ellipse((634,348,662,376),fill="#ab373a")
    d.rounded_rectangle((335,360,485,391),radius=6,fill="#303036")
    return im.crop(im.getbbox())
if __name__=="__main__":logo().save(sys.argv[1])
