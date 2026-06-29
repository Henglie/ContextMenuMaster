/* ============================================================
 *  FairyGlass · liquidGlass.js
 *  液态玻璃折射引擎（全浏览器统一）。
 *  ------------------------------------------------------------
 *  双模式加载（同一份文件，两种用法，统一通过 window.FairyGlass 取用）：
 *   1. 经典脚本：<script src="liquidGlass.js"></script>
 *      → window.FairyGlass.hydrateGlass(...)（webview / file:// 必走这条）
 *   2. ES 模块副作用导入：import "./liquidGlass.js"
 *      → 文件以模块身份运行，浏览器里 self===window，同样挂 window.FairyGlass，
 *        随后 const { hydrateGlass } = window.FairyGlass 取用。
 *
 *  为何不直接用 `export`：含 export 的文件被经典 <script> 加载时会抛
 *  SyntaxError 导致整段不执行，FairySave(file://) 就废了。故全程走 UMD +
 *  全局对象，一份文件通吃两种加载方式。
 *
 *  原理（区别于普通毛玻璃）：
 *    普通毛玻璃 = backdrop-filter: blur，把背景模糊掉。
 *    液态玻璃   = 用一张「位移贴图」描述每个像素的折射方向，通过 SVG
 *                <feDisplacementMap> 让玻璃边缘把背后的光斑真实弯曲/放大。
 *
 *  跨浏览器关键：折射走 filter:url() 而非 backdrop-filter:url()。
 *   - backdrop-filter:url() 折射「元素背后已渲染像素」→ 仅 Chromium 支持。
 *   - filter:url() 作用于「元素自身内容」→ 所有引擎都支持 feDisplacementMap。
 *   做法：给玻璃元素注入 .lg-refract 子层，该层用 background-attachment:fixed
 *   画一份与页面背景（--glow-field）按视口对齐的拷贝，再对这层施加位移图。
 *   于是「透过玻璃看到的光斑」与真实背景严丝合缝，仅边缘被弯曲，三系引擎一致。
 *
 *  位移贴图编码（color-interpolation-filters=sRGB）：
 *    R 通道 = X 位移，G 通道 = Y 位移，128 = 不位移（中性）。
 *    仅在距边缘 bezel 像素内的「斜面带」编码向量，中心保持 128 通透。
 * ============================================================ */
(function (root, factory) {
  const api = factory();
  // 统一出口：挂到全局 FairyGlass。
  //  - 经典 <script>（file:// / webview）：root === window，直接可用。
  //  - ES 模块 import "./liquidGlass.js"：浏览器里 self===window，同样挂上去。
  // 两种加载方式都不依赖 export，故文件里【绝不能出现 export 语句】——含 export
  // 的文件被经典 <script> 加载会抛 SyntaxError，整段工厂都不执行。
  root.FairyGlass = api;
  if (typeof module !== "undefined" && module.exports) module.exports = api;
})(typeof self !== "undefined" ? self : this, function () {
  "use strict";

  const SVG_NS = "http://www.w3.org/2000/svg";
  const XLINK = "http://www.w3.org/1999/xlink";
  let _host = null;
  let _uid = 0;

  /** 创建/复用承载所有 SVG 滤镜的隐藏容器 */
  function ensureHost() {
    if (_host) return _host;
    const svg = document.createElementNS(SVG_NS, "svg");
    svg.setAttribute("aria-hidden", "true");
    Object.assign(svg.style, {
      position: "absolute",
      width: "0",
      height: "0",
      overflow: "hidden",
      pointerEvents: "none",
    });
    document.body.appendChild(svg);
    _host = svg;
    return svg;
  }

  /** 圆角矩形有符号距离场：内部为负，外部为正，边缘为 0 */
  function sdRoundRect(px, py, w, h, r) {
    const qx = Math.abs(px - w / 2) - (w / 2 - r);
    const qy = Math.abs(py - h / 2) - (h / 2 - r);
    const ax = Math.max(qx, 0);
    const ay = Math.max(qy, 0);
    return Math.hypot(ax, ay) + Math.min(Math.max(qx, qy), 0) - r;
  }

  /**
   * 生成位移贴图 data URL。
   * 画布比元素大一圈（四周各留 bleed 边距）：折射层同样外扩 bleed 并被宿主圆角
   * 裁切，于是边缘往外位移时采样到的是真实光斑（来自 bleed 边距），而非层外空白
   * → 消除「内圈黑边」。元素形状在画布中居中（偏移 bleed）。
   */
  function buildDisplacementMap(w, h, radius, bezel, bleed) {
    const cw = w + bleed * 2;
    const ch = h + bleed * 2;
    const canvas = document.createElement("canvas");
    canvas.width = cw;
    canvas.height = ch;
    const ctx = canvas.getContext("2d");
    const img = ctx.createImageData(cw, ch);
    const data = img.data;
    const eps = 1;
    const sd = (px, py) => sdRoundRect(px - bleed, py - bleed, w, h, radius);

    for (let y = 0; y < ch; y++) {
      for (let x = 0; x < cw; x++) {
        const i = (y * cw + x) * 4;
        const dist = -sd(x, y); // 距边缘的内部深度
        let r = 128;
        let g = 128;

        if (dist >= 0 && dist < bezel) {
          // SDF 梯度 = 边缘外法线方向
          const gx = sd(x + eps, y) - sd(x - eps, y);
          const gy = sd(x, y + eps) - sd(x, y - eps);
          const len = Math.hypot(gx, gy) || 1;
          const nx = gx / len;
          const ny = gy / len;

          // 斜面折射强度：贴边最强，向内衰减到 0（凸面玻璃轮廓）
          const t = dist / bezel; // 0=边缘 1=斜面内沿
          const mag = Math.pow(1 - t, 1.6);

          r = 128 + nx * mag * 127;
          g = 128 + ny * mag * 127;
        }

        data[i] = r;
        data[i + 1] = g;
        data[i + 2] = 128;
        data[i + 3] = 255;
      }
    }

    ctx.putImageData(img, 0, 0);
    return canvas.toDataURL();
  }

  /**
   * 给元素应用液态玻璃折射。
   * @param {HTMLElement} el
   * @param {object} [opts]
   * @param {number} [opts.radius]     圆角（默认读元素 border-radius）
   * @param {number} [opts.bezel]      斜面带宽度，越大折射区越宽
   * @param {number} [opts.scale]      折射强度（位移像素），越大弯曲越猛
   * @param {number} [opts.blur]       轻微模糊，液态玻璃应很小
   * @param {number} [opts.dispersion] 色散强度（边缘彩虹镶边），0=关闭
   */
  function applyLiquidGlass(el, opts = {}) {
    const { bezel = 18, scale = 64, blur = 1.5, dispersion = 2.2 } = opts;
    ensureHost();

    const id = `lg-${_uid++}`;
    const filter = document.createElementNS(SVG_NS, "filter");
    filter.setAttribute("id", id);
    filter.setAttribute("color-interpolation-filters", "sRGB");
    filter.setAttribute("x", "-20%");
    filter.setAttribute("y", "-20%");
    filter.setAttribute("width", "140%");
    filter.setAttribute("height", "140%");

    const feImage = document.createElementNS(SVG_NS, "feImage");
    feImage.setAttribute("result", "map");
    feImage.setAttribute("preserveAspectRatio", "none");
    filter.appendChild(feImage);

    // —— 色散：RGB 三通道按不同强度折射 ——
    // 物理上短波（蓝）折射率更高、弯曲更多；长波（红）最少。
    const mkDisp = (sc, result) => {
      const d = document.createElementNS(SVG_NS, "feDisplacementMap");
      d.setAttribute("in", "SourceGraphic");
      d.setAttribute("in2", "map");
      d.setAttribute("xChannelSelector", "R");
      d.setAttribute("yChannelSelector", "G");
      d.setAttribute("scale", String(sc));
      d.setAttribute("result", result);
      return d;
    };
    // 仅保留单一通道，alpha 一律设为 1（SVG 滤镜用预乘 alpha，alpha=0 会乘没颜色）。
    const mkPick = (input, keep, result) => {
      const m = document.createElementNS(SVG_NS, "feColorMatrix");
      m.setAttribute("in", input);
      m.setAttribute("type", "matrix");
      const r = keep === "R" ? 1 : 0;
      const g = keep === "G" ? 1 : 0;
      const b = keep === "B" ? 1 : 0;
      m.setAttribute(
        "values",
        `${r} 0 0 0 0  0 ${g} 0 0 0  0 0 ${b} 0 0  0 0 0 1 0`
      );
      m.setAttribute("result", result);
      return m;
    };
    // screen 混合：通道不重叠时等价于相加，且 alpha 恒为 1
    const mkScreen = (a, b, result) => {
      const c = document.createElementNS(SVG_NS, "feBlend");
      c.setAttribute("in", a);
      c.setAttribute("in2", b);
      c.setAttribute("mode", "screen");
      c.setAttribute("result", result);
      return c;
    };

    if (dispersion > 0) {
      filter.append(
        mkDisp(scale + dispersion, "dR"), // 红：折射偏移量 +
        mkDisp(scale, "dG"),
        mkDisp(scale - dispersion, "dB"), // 蓝：折射偏移量 -
        mkPick("dR", "R", "pR"),
        mkPick("dG", "G", "pG"),
        mkPick("dB", "B", "pB"),
        mkScreen("pR", "pG", "rg"),
        mkScreen("rg", "pB", "out")
      );
    } else {
      filter.appendChild(mkDisp(scale, "out"));
    }
    _host.appendChild(filter);

    // 折射子层：承载背景拷贝，filter 施加于它（而非元素的 backdrop）
    let layer = el.querySelector(":scope > .lg-refract");
    if (!layer) {
      layer = document.createElement("span");
      layer.className = "lg-refract";
      el.insertBefore(layer, el.firstChild);
    }
    if (getComputedStyle(el).position === "static") el.style.position = "relative";
    el.classList.add("lg-host");

    const update = () => {
      const rect = el.getBoundingClientRect();
      const w = Math.round(rect.width);
      const h = Math.round(rect.height);
      if (w < 2 || h < 2) return;

      const cssR = parseFloat(getComputedStyle(el).borderTopLeftRadius) || 18;
      const radius = Math.min(opts.radius ?? cssR, w / 2, h / 2);
      const bz = Math.min(bezel, w / 2, h / 2);
      // bleed：足够容纳最大位移(≈scale/2)与模糊外溢。折射层与位移图同步外扩这么多，
      // 宿主 overflow:hidden 再按圆角裁回 → 边缘有真实光斑可采，无黑边。
      const bleed = Math.ceil(scale / 2 + blur + 4);

      const url = buildDisplacementMap(w, h, radius, bz, bleed);
      feImage.setAttribute("href", url);
      feImage.setAttributeNS(XLINK, "href", url);
      feImage.setAttribute("x", "0");
      feImage.setAttribute("y", "0");
      feImage.setAttribute("width", String(w + bleed * 2));
      feImage.setAttribute("height", String(h + bleed * 2));

      layer.style.inset = `-${bleed}px`;
      layer.style.filter = `blur(${blur}px) saturate(180%) url(#${id})`;
    };

    update();
    const ro = new ResizeObserver(update);
    ro.observe(el);
    return {
      update,
      destroy: () => {
        ro.disconnect();
        filter.remove();
        layer.remove();
        el.classList.remove("lg-host");
      },
    };
  }

  /** 是否支持 SVG 滤镜作为 filter（所有现代引擎，含 Firefox / WebView2） */
  function supportsLiquidGlass() {
    return typeof CSS !== "undefined" && CSS.supports("filter", "url(#x)");
  }

  /**
   * 扫描带 data-glass 的元素批量应用（幂等：已处理的跳过）。
   * preset 值：
   *   "button" — 主按钮/CTA，折射最强
   *   "chip"   — 小药丸/胶囊，折射轻
   *   "card"   — 浮空卡片/弹窗，中等（默认）
   *   "bar"    — 贴边长条（顶/底/侧栏）→ 跳过折射，保留磨砂
   */
  function hydrateGlass(root = document, opts = {}) {
    if (!supportsLiquidGlass()) return; // 极老浏览器降级：保留 glass.css 磨砂
    root.querySelectorAll("[data-glass]").forEach((el) => {
      if (el.dataset.lgInit === "1") return; // 已注入，避免重复
      const preset = el.getAttribute("data-glass");
      // 贴视口边缘的长条跳过折射：fixed 光斑图只在视口内，贴边那侧会采到视口外
      // 纯深色 → 黑边。这类元素保留磨砂，视觉仍统一。
      if (preset === "bar") {
        el.dataset.lgInit = "skip";
        return;
      }
      // 滚动容器跳过：inset 的折射层会随内容滚走且引发重绘。
      const ov = getComputedStyle(el).overflowY;
      if (ov === "auto" || ov === "scroll") {
        el.dataset.lgInit = "skip";
        return;
      }
      el.dataset.lgInit = "1";
      const presetOpts =
        preset === "button"
          ? { bezel: 22, scale: 72, dispersion: 3 }
          : preset === "chip"
          ? { bezel: 12, scale: 44, blur: 1, dispersion: 1.6 }
          : { bezel: 18, scale: 60, dispersion: 2.4 };
      applyLiquidGlass(el, { ...presetOpts, ...opts });
    });
  }

  return { applyLiquidGlass, supportsLiquidGlass, hydrateGlass };
});
