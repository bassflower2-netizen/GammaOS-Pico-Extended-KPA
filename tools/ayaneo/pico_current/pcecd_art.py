from PIL import Image, ImageDraw, ImageFont
def logo():
    im=Image.new("RGBA",(640,180))
    d=ImageDraw.Draw(im)
    d.text((320,90),"PC ENGINE CD",font=ImageFont.truetype("DejaVuSans-Bold.ttf",72),anchor="mm",fill="white")
    return im.crop(im.getbbox())
