import os
from IPython.display import HTML

def tex(name,timeout=10,force=False,width="100%",height="100%",
        align="center",alt="",border=0,display=display):
    
    # convert tex file to pdf
    if os.path.getmtime(f"{name}.tex") > os.path.getmtime(f"{name}.pdf"):
        os.system(f"pdflatex {name}.tex </dev/null 1>{name}.out 2>&1 & sleep {timeout} ; kill $! 2>/dev/null")

    # remove intermediate files
    for ext in ["aux","log","out"]: 
        if os.path.isfile(f"{name}.{ext}"): 
            os.remove(f"{name}.{ext}")

    # produce html
    html = HTML(f'<img src="./{name}.pdf" width="{width}" height="{height}" alt="{alt}" border="{border}" align="{align}"/>')
    if display == None:
        return html
    else:
        display(html)